#ifndef MIMEPARSER_H
#define MIMEPARSER_H

#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>

static int findStr(const char* src, const char*ptn, int lenSrc = 0, int sPos = 0) {
    int pos = -1;
    lenSrc = lenSrc == (unsigned int) 0 ? strlen(src) : lenSrc;
    int lenPtn = strlen(ptn);
    if (sPos < 0 || lenSrc <=0 || lenPtn <= 0 || lenSrc - sPos < lenPtn) {
        return pos;
    }

    for (int idx = sPos; idx <= lenSrc - lenPtn; idx++) {
        if (0 == memcmp(src + idx, ptn, lenPtn)) {
            pos = idx;
            break;
        }
    }

    return pos;
}

static std::vector<std::string> Split(const std::string& s, const std::string& c, bool skipEmptyPart = true, int maxCnt = -1) {
    std::vector<std::string> v;
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while (std::string::npos != pos2) {
        if (!(pos2 == pos1 && skipEmptyPart))
            v.push_back(s.substr(pos1, pos2 - pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);

        if ( (int)v.size() == maxCnt - 1) {
            break;
        }
    }

    if (pos1 != s.length()) {
        v.push_back(s.substr(pos1));
    }

    return v;
}


inline void Ltrim(std::string& s) {
    if (s.size() == 0) return;

    int pos = -1;
    for (size_t idx = 0; idx < s.size(); idx++) {
        if (s.at(idx) == ' ') {
            pos = idx;
        }
        else {
            break;
        }
    }
    if (pos != -1) {
        s.erase(0, pos + 1);
    }
}

inline void Rtrim(std::string& s) {
    if (s.size() == 0) return;

    int pos = -1;
    for (int idx = (int)s.size() - 1; idx >= 0; idx--) {
        if (s.at(idx) == ' ') {
            pos = idx;
        }
        else {
            break;
        }
    }

    if (pos != -1) {
        s.erase(pos);
    }
}

inline void Trim(std::string& s) {
    Ltrim(s);
    Rtrim(s);
}



struct AttachmentRestorBuffer {
    AttachmentRestorBuffer() {
    }
    void init(int eachAtchBufLen) {
        pBuf = new char[eachAtchBufLen];
        bufLen = eachAtchBufLen;
    }
    ~AttachmentRestorBuffer() {
        if (nullptr != pBuf) {
            delete [] pBuf;
        }
    }
    int offset {0};
    int writtenLen {0};
    int bufLen {0};
    char * pBuf {nullptr};
#define AttachNameLen 1024
    char attachName[AttachNameLen] {0, };
};

struct MIMETree {
    std::string messageContent;
    std::map<std::string, std::string> heads;
    std::map<std::string, std::string> fields;
    std::vector<struct MIMETree*> children;
    struct MIMETree* parent = NULL;
    //	std::string boundary;
    int continouslineBreakCnt;

    std::string mLastHeadKey;

    //if is true, current data is part of data stream
    bool indatastream;

    //file handler
    FILE* fp;

    explicit MIMETree()
        :parent(NULL), continouslineBreakCnt(0), indatastream(false), fp(NULL)
    {

    }

    ~MIMETree() {
        if (NULL != this->fp) fclose(fp);

        //        fp = NULL;

        for (auto ptr: children) {
            delete ptr;
        }
    }

    inline void SetHeads(const std::string& k, const std::string& v) {
        heads.insert(std::pair<std::string, std::string>(k, v));
    }

    inline std::string& GetHeads(const std::string& k) {
        return this->heads[k];
    }

    inline void SetFields(const std::string& k, const std::string& v) {
        fields.insert(std::pair<std::string, std::string>(k, v));
    }

    inline const std::string& GetFields(const std::string& k) {
        return fields[k];
    }

    inline int parseHeadAndField(const char* data, size_t lenData, std::pair<std::string, std::string>* headPair = nullptr,
                                 std::pair<std::string, std::string>* fieldPair = nullptr) {

        bool isSubHead {false};
        if (lenData > 0 && ('\t' == data[0] || ' ' == data[0]) ) {
            isSubHead = true;
        }

        int pos = findStr(data, ":", lenData);
        if (pos < 0 || isSubHead) {
            if (!mLastHeadKey.empty()) {
                std::string& val = GetHeads(mLastHeadKey);
                val += "\r\n" + std::string(data, lenData);
            }

            pos = findStr(data, "=", lenData);
            if (pos <= 2 || int(lenData) - pos -3 <= 0) {
                //Don't care
                return 0;
            }

            std::string fieldStr(data, lenData);
            //Trim whitespace
            Trim(fieldStr);
            //Trim tab character
            while (fieldStr.size() && fieldStr[0] == '\t') {
                fieldStr = fieldStr.substr(1);
            }

            auto strLst = Split(fieldStr, "=", true, 2);
            if (strLst.size() != 2) {
                //Don't care
                return 0;
            }

            auto fieldVal = strLst[1];
            if (fieldVal.size() && fieldVal[0] == '"') {
                fieldVal = fieldVal.substr(1);
            }

            if (fieldVal.size() && fieldVal[fieldVal.size() -1] == '"') {
                fieldVal = fieldVal.substr(0, fieldVal.size() -1);
            }

            this->SetFields( mLastHeadKey + ":" + strLst[0], fieldVal);

            if (fieldPair != nullptr) {
                *fieldPair = std::pair<std::string, std::string> (mLastHeadKey + ":" + strLst[0], fieldVal);
            }

        } else {
            if (int(lenData) - pos - 2 <= 0) {
                //ERROR, invalid format
                return -2;
            }
            std::string k(data, pos);
            std::string val(data + pos + 2, lenData - pos - 2);
            this->SetHeads(k, val);
            if (headPair != nullptr) {
                *headPair = std::pair<std::string, std::string> (k, val);
            }

            auto strLst = Split(val, ";");
            for (auto& str: strLst) {
                Trim(str);
                auto strLst = Split(str, "=", true, 2);
                if(strLst.size() == 2) {
                    auto fieldVal = strLst[1];
                    if (fieldVal.size() > 2) {
                        if (fieldVal[0] == '"') {
                            fieldVal = fieldVal.substr(1);
                        }

                        if (fieldVal[fieldVal.size() - 1] == '"') {
                            fieldVal = fieldVal.substr(0, fieldVal.size() -1);
                        }
                    }

                    SetFields(k + ":" + strLst[0], fieldVal);
                }
            }

            mLastHeadKey = k;
        }

        return 0;
    }
};


class MIMEParser {

public:
    MIMEParser();
    ~MIMEParser();

    void init();
    void parseMIME(char* data, size_t lenData);
    std::string getMailBody() {
        std::string retStr;
        std::string retStrHtml;
        if (nullptr != rootTree) {
            for (const auto a: rootTree->children) {
                for (const auto aa: a->children) {
                    if (findStr(aa->GetHeads("Content-Type").c_str(), "text/plain") == 0) {
                        retStr = aa->messageContent;
                    }

                    if (findStr(aa->GetHeads("Content-Type").c_str(), "text/html") == 0) {
                        retStrHtml = aa->messageContent;
                    }
                }

                if (findStr(a->GetHeads("Content-Type").c_str(), "text/plain") == 0) {
                    retStr = a->messageContent;
                }

                if (findStr(a->GetHeads("Content-Type").c_str(), "text/html") == 0) {
                    retStrHtml = a->messageContent;
                }
            }
        }

        return (retStr.empty() ? std::string() : retStr + "\r\n") + retStrHtml;
    }

    //parsing line by line
    // if use the function directlly, don't regardless empty line
    void parseOneLine(char* data, size_t lenData);

    //if doesn't restore attachment to disk, so restore attachment to memory
    //number of restoring to memory
    int restoreAtchmtNum {10};
    int eachAtchBufLen {4*1024*1024};

    bool restoreAttachment {false};
    bool restoreAttchmtToDisk {false};
    std::string attchmtRestorPath;
    AttachmentRestorBuffer* pAttachmentBufferStart;
    std::map<std::string, std::string> envelope;

    bool restoreMessageBody {false};

    char buf[2048]{0, };
    size_t offset {0};
    MIMETree* curMimeTree{nullptr};

    MIMETree* rootTree {nullptr};
    bool inMail{false};
    bool endflag{false};
    std::string mailFlagLine;

    bool ParseMIMEEnd {false};

private:
    bool parsingEnvlpFnshed{false};
};

#endif //MIMEPARSER_H
