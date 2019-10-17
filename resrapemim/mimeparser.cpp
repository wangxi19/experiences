#include "mimeparser.h"
#include <re2/re2.h>
#include <iostream>

#ifndef MIN
#define MIN(a, b)	((a) > (b) ? (b) : (a))
#endif //MIN

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif //MAX


//下表中，“+”==“-”，“/”==“_”,兼容URL Safe
static int BASE64_DECODE_TABLE[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, //  00 -  07
    255, 255, 255, 255, 255, 255, 255, 255, //  08 -  15
    255, 255, 255, 255, 255, 255, 255, 255, //  16 -  23
    255, 255, 255, 255, 255, 255, 255, 255, //  24 -  31
    255, 255, 255, 255, 255, 255, 255, 255, //  32 -  39
    255, 255, 255,  62, 255, 62, 255,  63, //  40 -  47
    52,  53,  54,  55,  56,  57,  58,  59, //  48 -  55
    60,  61, 255, 255, 255, 255, 255, 255, //  56 -  63
    255,	0,	1,	2,	3,	4,	5,	6, //  64 -  71
    7,	8,	9,  10,  11,  12,  13,  14, //  72 -  79
    15,  16,  17,  18,  19,  20,  21,  22, //  80 -  87
    23,  24,  25, 255, 255, 255, 255, 63, //  88 -  95
    255,  26,  27,  28,  29,  30,  31,  32, //  96 - 103
    33,  34,  35,  36,  37,  38,  39,  40, // 104 - 111
    41,  42,  43,  44,  45,  46,  47,  48, // 112 - 119
    49,  50,  51, 255, 255, 255, 255, 255, // 120 - 127
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255
};

static int Base64Decode(char *pDest, int outlen, char *pSrc, int nSize)
{
    unsigned int lByteBuffer, lByteBufferSpace;
    unsigned int C;
    intptr_t reallen;
    char *InPtr, *InLimitPtr, *OutLimitPtr;
    char *OutPtr;

    if ((pSrc == NULL) || (pDest == NULL) || (nSize <= 0))
        return 0;

    lByteBuffer = 0;
    lByteBufferSpace = 4;

    InPtr = pSrc;
    InLimitPtr= InPtr + nSize;
    OutLimitPtr = InPtr + outlen;
    OutPtr = pDest;

    while ((InPtr != InLimitPtr) && (OutPtr != OutLimitPtr))
    {
        C = BASE64_DECODE_TABLE[(int)*InPtr];
        InPtr++;
        if (C == 0xFF)
            continue;
        lByteBuffer = lByteBuffer << 6 ;
        lByteBuffer = lByteBuffer | C ;
        lByteBufferSpace--;
        if (lByteBufferSpace != 0)
            continue;
        OutPtr[2] = lByteBuffer;
        lByteBuffer = lByteBuffer >> 8;
        OutPtr[1] = lByteBuffer;
        lByteBuffer = lByteBuffer >> 8;
        OutPtr[0] = lByteBuffer;

        OutPtr += 3; lByteBuffer = 0; lByteBufferSpace = 4;
    }
    reallen = (intptr_t)OutPtr - (intptr_t)pDest;

    switch (lByteBufferSpace)
    {
    case 1:
        if( (reallen + 3 ) > outlen )
        {
            return reallen;
        }
        lByteBuffer = lByteBuffer >> 2;
        OutPtr[1] = lByteBuffer;
        lByteBuffer = lByteBuffer >> 8;
        OutPtr[0] = lByteBuffer;
        OutPtr[2] = 0;
        return reallen + 2;
    case 2:
        if( (reallen + 2 ) > outlen )
        {
            return reallen;
        }
        lByteBuffer = lByteBuffer >> 4;
        OutPtr[0] = lByteBuffer;
        OutPtr[1] = 0;
        return reallen + 1;
    default:
        if( reallen >= outlen )
        {
            return reallen;
        }
        OutPtr[0] = 0;
        return reallen;
    }
}

MIMEParser::MIMEParser()
{

}

MIMEParser::~MIMEParser()
{
    if (rootTree != nullptr) {
        delete rootTree;
    }
}

void MIMEParser::init() {
    if (restoreAttachment && !restoreAttchmtToDisk) {
        pAttachmentBufferStart = new AttachmentRestorBuffer[restoreAtchmtNum];
        for (int i = 0; i < restoreAtchmtNum; i++) {
            (pAttachmentBufferStart + i)->init(eachAtchBufLen);
        }
    }
}

void MIMEParser::parseOneLine(char* data, size_t lenData) {
    //Don't waste efficiency if don't have to restore attachment
    if (parsingEnvlpFnshed && !restoreAttachment && !restoreMessageBody) {
        return;
    }

    if (ParseMIMEEnd) {
        return;
    }

    if (NULL == curMimeTree && !ParseMIMEEnd) {
        curMimeTree = new MIMETree;
        if (NULL == curMimeTree) {
            //ERROR, memory is exhausted
            return;
        }

        if (rootTree != nullptr) {
            delete curMimeTree;
            curMimeTree = nullptr;
            return;
        }

        rootTree = curMimeTree;
    }

    if (lenData == 0) {
        curMimeTree->continouslineBreakCnt += 1;
        return;
    }
    int clbc = curMimeTree->continouslineBreakCnt;
    curMimeTree->continouslineBreakCnt = 0;

    bool isMsgOrNetPart = clbc > 0 || curMimeTree->indatastream;

    if (!isMsgOrNetPart) {
        int rst;
        if (rootTree == curMimeTree) {
            std::pair<std::string, std::string> headPair;
            rst = curMimeTree->parseHeadAndField(data, lenData, &headPair);
            if (!headPair.first.empty())  envelope.insert(headPair);

        } else {
            rst = curMimeTree->parseHeadAndField(data, lenData);
        }
        if (0 != rst) {
            //ERROR
            return;
        }
        return;
    }

    if (rootTree == curMimeTree) {
        parsingEnvlpFnshed = true;
        envelope = rootTree->heads;

        //Don't waste efficiency if don't have to restore attachment
        if (!restoreAttachment && !restoreMessageBody) {
            return;
        }
    }

    bool isContent = false;
    if (isMsgOrNetPart) {
        curMimeTree->indatastream = isMsgOrNetPart;
        //maybe next part
        if (0 == findStr(data, "--", lenData) && lenData > 2) {
            std::string cntType = curMimeTree->GetHeads("Content-Type");
            if (cntType.size() <= 0) {
                //ERROR, No Content-Type has been specified
                return;
            }

            bool isMultiPart = findStr(cntType.c_str(), "multipart/", cntType.size()) == 0;
            std::string boundary = curMimeTree->GetFields("Content-Type:boundary");

            bool hasBoundary = boundary.size() > 0;

            if (isMultiPart != hasBoundary) {
                //ERROR, No Boundary has been specified
                return;
            }

            //must have a none multipart type part between two boundary
            if (isMultiPart) {
                //lenData - 4 != boundary.size() , boundary terminate
                if (lenData - 2 != boundary.size()) {
                    //isn't next part, just is massage
                    isContent = true;
                    goto out;
                }

                if (0 != memcmp(data + 2, boundary.c_str(), lenData - 2)) {
                    //isn't next part, just is massage
                    isContent = true;
                    goto out;
                }

                MIMETree* child = new MIMETree;
                if (NULL == child) {
                    //ERROR, memory is exhausted
                    return;
                }
                curMimeTree->children.push_back(child);
                child->parent = curMimeTree;
                curMimeTree = child;
            } else {
                MIMETree* parent = curMimeTree->parent;
                if (NULL == parent) {
                    //isn't next part, just is massage
                    isContent = true;
                    goto out;
                }

                std::string boundary = parent->GetFields("Content-Type:boundary");
                /*
                 * boundary size is never less than zero
                if (boundary.size() <= 0) {
                    //isn't next part, just is massage
                    return;
                }
                 */


                //both is not boundary and boundary end characters
                if (lenData - 2 != boundary.size() && lenData - 4 != boundary.size()) {
                    //isn't next part, just is massage
                    isContent = true;
                    goto out;
                }

                if (lenData - 2 == boundary.size()) {
                    if (0 != memcmp(data + 2, boundary.c_str(), lenData - 2)) {
                        //isn't next part, just is massage
                        isContent = true;
                        goto out;
                    }

                    //Go to next part(isn't going to child part), so delete current part
//                    parent->children.pop_back();
                    MIMETree* child = new MIMETree;
                    if (NULL == child) {
                        //ERROR, memory is exhausted
                        return;
                    }

                    parent->children.push_back(child);
                    child->parent = parent;
//                    delete curMimeTree;
                    curMimeTree = child;
                } else if (lenData - 4 == boundary.size()) {
                    std::string tmp = boundary + "--";
                    if (0 != memcmp(data + 2, tmp.c_str(), lenData - 2)) {
                        //isn't next part, just is massage
                        isContent = true;
                        goto out;
                    }

                    //Terminate current part and current part's parent part
//                    delete curMimeTree;
                    MIMETree* pMimeTree = parent->parent;
//                    delete parent;
                    curMimeTree = pMimeTree;
                    if (NULL != curMimeTree) {
//                        curMimeTree->children.pop_back();
                    }
                    if (nullptr == curMimeTree /*&& parent == rootTree*/) ParseMIMEEnd = true;


                }
            }
        } else {
            isContent = true;
        }
    }

out:
    if (isContent) {
        const std::string& ContentDisposition = curMimeTree->heads["Content-Disposition"];
        std::string AttachName(curMimeTree->fields["Content-Disposition:filename"].c_str());
        if (restoreAttachment
                && AttachName.size() > 0
                && AttachName.size() < 255
                && 0 == findStr(ContentDisposition.c_str(), "attachment")
                )
        {
            char buffer[lenData];
            char* buf = buffer;
            memset(buf, '\0', lenData);
            int decodedLen;

            //Decode the encoded data
            //Support base64 only
            if (findStr(curMimeTree->GetHeads("Content-Transfer-Encoding").c_str(), "base64") == 0) {
                decodedLen = Base64Decode(buf, lenData, data, lenData);
            } else if (!curMimeTree->GetHeads("Content-Transfer-Encoding").empty()) {
                AttachName = AttachName + "." + curMimeTree->GetHeads("Content-Transfer-Encoding");
                decodedLen = lenData;
                buf = data;
            } else {
                decodedLen = lenData;
                buf = data;
            }

            if (restoreAttchmtToDisk && NULL == curMimeTree->fp) {
                if (0 == access(attchmtRestorPath.c_str(), W_OK)) {
                    curMimeTree->fp = fopen((attchmtRestorPath + "/" + AttachName).c_str(), "w+");
                    if (NULL == curMimeTree->fp) {
                        //ERROR, Opening file to write failed
                        return;
                    }
                }
            }

            if (restoreAttchmtToDisk) {
                fwrite(buf, 1, decodedLen, curMimeTree->fp);
            } else {
                for (int i = 0; i < restoreAtchmtNum; i++) {
                    auto ptr = pAttachmentBufferStart + i;
                    if (ptr->writtenLen == 0 || 0 == memcmp(ptr->attachName, AttachName.c_str(), AttachName.size() <= AttachNameLen ? AttachName.size() : AttachNameLen)) {
                        int hopeWrtingLen = decodedLen;
                        char* orignBuf = buf;

                        while (true) {
                            int wrtlen = (ptr->bufLen - ptr->offset) >= hopeWrtingLen ? hopeWrtingLen : (ptr->bufLen - ptr->offset);
                            memcpy(ptr->pBuf + ptr->offset, orignBuf, wrtlen);
                            ptr->offset = (ptr->offset + wrtlen) % ptr->bufLen;
                            ptr->writtenLen += wrtlen;
                            hopeWrtingLen -= wrtlen;

                            if (hopeWrtingLen <= 0) break;

                            orignBuf += wrtlen;
                        }


                        if (ptr->attachName[0] == '\0') {
                            snprintf(ptr->attachName, AttachNameLen, "%s", AttachName.c_str());
                        }
                        break;
                    }

                }
            }


        }

        //Restore message body
        if (restoreMessageBody && 0 != findStr(ContentDisposition.c_str(), "attachment")) {
            char buffer[lenData];
            char* buf = buffer;
            memset(buf, '\0', lenData);
            int decodedLen;

            if (findStr(curMimeTree->GetHeads("Content-Transfer-Encoding").c_str(), "base64") == 0) {
                decodedLen = Base64Decode(buf, lenData, data, lenData);
            } else {
                decodedLen = lenData;
                buf = data;
            }

            curMimeTree->messageContent += std::string(buf, decodedLen);
        }
    }
}

void MIMEParser::parseMIME(char* iData, size_t iLenData) {
    size_t lenData = iLenData;
    char* data = iData;

    if (lenData == 0) {
        return;
    }

    while (true) {
        int pos = findStr(data, "\r\n", lenData);
        pos = pos < 0 ? lenData : pos;
        if (sizeof buf - offset < (size_t) pos) {
            //error, one line size is greater than 1024
            return;
        }

        memcpy(buf + offset, data, pos);
        offset += pos;

        if ((size_t) pos < lenData) {
            //composed one line data
            if (inMail) {
                parseOneLine(buf, offset);
            } else {
                std::string bufStr(buf, offset);
                if (RE2::PartialMatch(bufStr.c_str(), "^\\*\\s\\d+\\sFETCH\\s"))  {
                    inMail = true;
                }else if (RE2::PartialMatch(bufStr.c_str(), "^\\+OK\\s\\d+\\s(octets)?")) {
                    inMail = true;
                }else {
                    if (offset == 4 && 0 == memcmp(buf, "DATA", 4)) {
                        mailFlagLine = "DATA";
                    } else if (mailFlagLine == "DATA" && offset > 3 && 0 == memcmp(buf, "354 ", 4)) {
                        inMail = true;
                    } else {
                        mailFlagLine.clear();
                    }
                }

            }
            offset = 0;

            lenData -= (pos+2);
            data += (pos+2);

            continue;
        }
        break;
    }
}
