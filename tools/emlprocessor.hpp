#ifndef EMLPARSER_HPP
#define EMLPARSER_HPP
#define NOLOG

//parse mime text
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <map>
#include <mimetic/mimetic.h>
#include <pthread.h>
#include <rollBuffer.h>
#include <tuple>
#include <curl.hpp>
#include <time.h>
#include <iconv.h>
#include <re2/re2.h>

#ifndef NOLOG
#include <baselog.h>
#endif

#include <QByteArray>
using namespace std;
using namespace mimetic;

#define FBUFSZ (1024*1024*20)
#define BDBUFSZ (1024*30)
#define DBBUFSZ (1024*1024*40)

class EmlProcessor {
public:
    enum PrivateIPType {T192,T172,T10,TInvalid = 1000};
    enum CommIPType {T1_64, T65_128, T129_192, T193_255};
public:
    explicit EmlProcessor() {

    }

    void Init(const std::string& iESDBHostPort = "http://127.0.0.1:9200/", uint32_t size = 2000) {
        mRollBuffer.Init((int)size);
        fBufPtr = (char*)_malloc(FBUFSZ);
        dbBufPtr = (char*)_malloc(DBBUFSZ);
        mESDBHostPort = iESDBHostPort;
        mInited = true;
    }

    ~EmlProcessor() {
        for (const auto ptr: mallocLst) {
            if (ptr != NULL) {
                free(ptr);
            }
        }
        mallocLst.clear();
    }

    //all of these below ip addresses are using net endian
    bool addEml(const char* filepath, uint32_t srcip, uint32_t dstip, uint32_t date) {
        if (!mInited) {
            return false;
        }

        std::string str(filepath);
        if (str.size() <= strlen(".eml") || strcmp(str.substr(str.size() - 1 - 3).c_str(), ".eml") != 0) {
            return -1;
        }

        int fd = open(filepath, O_RDONLY);
        if (fd <= 0) {
            return -1;
        }

        if (fBufPtr == NULL) {
            fBufPtr = (char*)_malloc(FBUFSZ);
        }

        auto rdsz = read(fd, fBufPtr, FBUFSZ);
        close(fd);

        auto retvec = EmlPreProcessPlus(fBufPtr, rdsz);

        for (const auto& tp: retvec) {
            auto _ = this->mRollBuffer.getWriteBucket(0);
            if (_ == NULL) {
                mLosCount++;
                continue;
            }
            auto& bdBfrPtr = std::get<4>(*_);
            if (bdBfrPtr == NULL) {
                bdBfrPtr = (char*)_malloc(BDBUFSZ);
            }
            auto& envelopeMp = std::get<3>(*_);
            envelopeMp.clear();
            auto& oBodySz = std::get<5>(*_);

            istringstream s1(string(fBufPtr+std::get<0>(tp), (size_t) (std::get<1>(tp)-std::get<0>(tp)+1)));
            istream s2(s1.rdbuf());

            MimeEntity me(s2);

            Header& h = me.header();
            envelopeMp["From"] = h.from().str();
            envelopeMp["To"] = h.to().str();
            envelopeMp["Date"] = h.field("Date").value();
            envelopeMp["Received"] = h.field("Received").value();
            envelopeMp["Subject"] = h.field("Subject").value();
            envelopeMp["Reply-To"] =  h.field("Reply-To").value();
            envelopeMp["Content-Transfer-Encoding"] = h.field("Content-Transfer-Encoding").value();
            parseMimeStructure(&me, bdBfrPtr, oBodySz);

            auto& _0 = std::get<0>(*_);
            auto& _1 = std::get<1>(*_);
            auto& _2 = std::get<2>(*_);
            auto& protoc = std::get<6>(*_);


            _0 = srcip;
            _1 = dstip;
            _2 = date;
            strncpy(protoc, std::get<2>(tp).c_str(), sizeof(protoc));

            if (envelopeMp["From"].size() > 0 && envelopeMp["To"].size() > 0 && oBodySz > 0) {
                this->mRollBuffer.Write();
                mInsCount++;
            } else {
                this->mRollBuffer.undoGetWriteBucket();
            }
        }

        if (retvec.size() == 0) {
            mFsParseCount++;
        }

        return retvec.size()>0;
    }

    void* _malloc(size_t sz) {
        void* ret = malloc(sz);
        mallocLst.push_back(ret);
        return ret;
    }
public:
    // all of these below ip addresses are using net endian
    //               srcip   dstip       date        envelope        body    bodysz  protocol
    typedef tuple<uint32_t, uint32_t, uint32_t, map<string, string>, char*, size_t, char[8]> EmlItem;
    CRollBuffer<EmlItem> mRollBuffer;
    std::vector<void*> mallocLst;
    char* fBufPtr{NULL};
    char* dbBufPtr{NULL};
    std::string mESDBHostPort{"http://127.0.0.1:9200/"};
    bool mInited{false};
    uint64_t mLosCount{0};
    uint64_t mInsCount{0};
    uint64_t mFsParseCount{0};
public:
    static bool TimerTaskPfn(void* arg) {
        auto tsPtr = (EmlProcessor*) arg;
        tsPtr->pfnPthread(arg);
        return true;
    }

    static void* pfnPthread(void* arg) {
        auto tsPtr = (EmlProcessor*) arg;
        if (!tsPtr->mInited) {
            return 0;
        }

        if (tsPtr->dbBufPtr == NULL) {
            return 0;
        }

        char todayBuf[16]{0,};
        size_t offset {0};
        size_t offsetPre {0};
        while (true) {
            offsetPre = offset;
            auto ___ = tsPtr->mRollBuffer.getReadBucket(0);
            if (___ == NULL) {
                break;
            }

            auto& tp = *___;

            if (!GetTody(todayBuf, 16, std::get<2>(tp))) {
#ifndef NOLOG
                BYBASE_LOG(ERROR, "Fail to get today, %u\n", std::get<2>(tp));
#endif
                tsPtr->mRollBuffer.Read();
                continue;
            }

            //            auto _ = isPrivateIp(std::get<0>(tp));
            std::string bulkIndex{R"({"index":{"_index":"mails_)"};
            //            if (!std::get<0>(_)) {
            auto __ = getCommIPType(std::get<0>(tp));
            if (__ == CommIPType::T1_64) {
                bulkIndex += "1_64_";
            } else if (__ == CommIPType::T65_128) {
                bulkIndex += "65_128_";
            } else if (__ == CommIPType::T129_192) {
                bulkIndex += "129_192_";
            } else if (__ == CommIPType::T193_255) {
                bulkIndex += "193_255_";
            }
            //            } else {
            //                bulkIndex += "private_";
            //            }

            bulkIndex += todayBuf;
            bulkIndex += R"("}})";
            bulkIndex += "\n";

            if (offset + bulkIndex.size() >= DBBUFSZ) {
                if (offsetPre > 0) {
                    SyncToDb(tsPtr->dbBufPtr, offsetPre,  tsPtr->mESDBHostPort + "_bulk");
                }
                auto _pre = offsetPre;
                offset = 0;
                offsetPre = 0;
                tsPtr->mRollBuffer.undoGetReadBucket();
                if (_pre > 0) {
                    continue;
                } else {
                    break;
                }
            }

            auto snpret1 = snprintf(tsPtr->dbBufPtr + offset, (DBBUFSZ - offset), bulkIndex.c_str());
            if (snpret1 < 0) {
                offset = offsetPre;
                tsPtr->mRollBuffer.Read();
                continue;
            }
            offset += (size_t)snpret1;

            std::string doc;
            EmlItemToJson(tp, doc);

            if (offset + doc.size() > DBBUFSZ) {
                if (offsetPre > 0) {
                    SyncToDb(tsPtr->dbBufPtr, offsetPre,  tsPtr->mESDBHostPort + "_bulk");
                }
                auto _pre = offsetPre;
                offset = 0;
                offsetPre = 0;
                tsPtr->mRollBuffer.undoGetReadBucket();
                if (_pre > 0) {
                    continue;
                } else {
                    break;
                }
            }

            memcpy(tsPtr->dbBufPtr + offset, doc.c_str(), doc.size());
            offset += doc.size();
            tsPtr->mRollBuffer.Read();
        }

        if (offset > 0) {
            SyncToDb(tsPtr->dbBufPtr, offset,  tsPtr->mESDBHostPort + "_bulk");
        }
        return 0;
    }

    static void SyncToDb(char* data, size_t datalen, const std::string& index) {
        CURLNS::POST(index.c_str(), data, datalen);
    }

    static void EmlItemToJson(const EmlItem& emlItem, std::string& oJson) {
        char buf[24]{0, };
        GetTodyDateTime(buf, sizeof(buf), std::get<2>(emlItem));
        const auto& envelope = std::get<3>(emlItem);
        std::string from, to;
        auto ite = envelope.find("From");
        if (ite != envelope.end()) {
            from = ite->second;
        }
        ite = envelope.find("To");
        if (ite != envelope.end()) {
            to = ite->second;
        }

        oJson += R"({"srcip":")";
        oJson += ip2str(std::get<0>(emlItem));
        oJson += R"(","dstip":")";
        oJson += ip2str(std::get<1>(emlItem));
        oJson += R"(","protocol":")" + std::string(std::get<6>(emlItem)) + "\"";
        oJson += R"(,"date":)"+std::to_string(std::get<2>(emlItem));
        oJson += R"(,"date_h":")";
        oJson += buf;
        oJson += R"(","from":)" + quote(from);
        oJson += R"(,"to":)" + quote(to);
        oJson += R"(,"envelope":)" + mapToJson(envelope);
        oJson += R"(,"mail_body":)" + quote(std::string(std::get<4>(emlItem), std::get<5>(emlItem)), (int)std::get<5>(emlItem), 1);
        oJson += R"(})";
        oJson += "\n";
    }

    static std::string mapToJson(const std::map<std::string,std::string>& mp) {
        std::string oJson("{");
        for (const auto& ite: mp) {
            oJson += quote(ite.first) + ":";
            oJson += quote(ite.second) + ",";
        }
        if (oJson[oJson.size()-1] == ',') {
            oJson[oJson.size()-1] = '}';
        } else {
            oJson += "}";
        }

        return oJson;
    }

    static inline char* ip2str(uint32_t ipNetEndian, char* buf = NULL) {
        char* ret;
        in_addr addr;
        addr.s_addr = ipNetEndian;
        ret = inet_ntoa(addr);
        if (buf)
            sprintf(buf,"%s", ret);

        return ret;
    }

    static std::string quote(const std::string& iString, int len = -1, int flag = 0) {
        if (iString.size() == 0) {
            return "\"\"";
        }

        char c = 0;
        int i;
        if (len == -1) len = (int)iString.length();

        std::string retStr;

        retStr += '"';
        char hexBuf[8]{0, };
        for (i = 0; i < len; i += 1) {
            c = iString.at(i);
            switch (c) {
            case '\\':
            case '"':
            case '/':
                retStr += '\\';
                retStr += c;
                break;
            case '\b':
                retStr.append("\\b");
                break;
            case '\t':
                retStr.append("\\t");
                break;
            case '\n':
                retStr.append("\\n");
                break;
            case '\f':
                retStr.append("\\f");
                break;
            case '\r':
                retStr.append("\\r");
                break;
            default:
                if (c < ' ' && flag == 0) {
                    sprintf(hexBuf, "%04x", c);
                    retStr += "\\u";
                    retStr += hexBuf;
                } else {
                    retStr += c;
                }
            }
        }
        retStr += '"';
        return retStr;
    }

    static size_t codeConvert(const char* srcCharset, const char* dstCharset, char* inbuf, size_t inlen, char* outbuf, size_t outlen) {
        iconv_t cd;
        char** pin = &inbuf;
        char** pout = &outbuf;
        cd = iconv_open(dstCharset, srcCharset);
        if (cd == 0) {
            return (size_t)-1;
        }

        auto ret = iconv(cd, pin, &inlen, pout, &outlen);
        iconv_close(cd);
        return ret;
    }

public:
    //eml tools
    static void parseMimeStructure(MimeEntity* pMe, char* oBody, size_t& oBodySz, int tabcount = 0) {
        Header& h = pMe->header();
        std::string charset = h.contentType().param("charset");
        std::string ctype = h.field("Content-Type").value();
        if (ctype.size() > 4 && strncmp(ctype.c_str(), "text", 4) == 0) {
            if (re2::RE2::PartialMatch(h.field("Content-Transfer-Encoding").value(), "^[Bb][Aa][Ss][Ee]64$")) {
                QByteArray bt = QByteArray::fromBase64(pMe->body().c_str());
                if (bt.size() > 0) {
                    oBodySz = bt.size() > (BDBUFSZ-1) ? (BDBUFSZ-1) : bt.size();
                    strncpy(oBody, bt.data(), oBodySz);
                    oBody[oBodySz] = 0;
                    if (!re2::RE2::PartialMatch(charset, "^[Uu][Tt][Ff]\-?8$")) {
                        char* _ = (char*)malloc(oBodySz*2);
                        memset(_, 0, oBodySz*2);
                        if (codeConvert(charset.c_str(), "utf-8", oBody, oBodySz, _, oBodySz*2) != (size_t)-1) {
                            size_t _Length = strlen(_);
                            oBodySz = _Length > (BDBUFSZ-1) ? (BDBUFSZ-1) : _Length;
                            strncpy(oBody, _, oBodySz);
                            oBody[oBodySz] = 0;
                        }
                        free(_);
                    }
                    //                Base64::Decoder b64d;
                    //                decode(pMe->body().c_str(), pMe->body().c_str() + pMe->body().size(), b64d, oBody);
                } else {
                    oBodySz = 0;
                    oBody[oBodySz] = 0;
                }
            } else {
                oBodySz = pMe->body().size() > (BDBUFSZ-1) ? (BDBUFSZ-1) : pMe->body().size();
                strncpy(oBody, pMe->body().c_str(), oBodySz);
                oBody[oBodySz] = 0;
            }
            return;
        }

        MimeEntityList& parts = pMe->body().parts();
        MimeEntityList::iterator mbit = parts.begin(), meit = parts.end();
        for (; mbit != meit; ++mbit) {
            parseMimeStructure(*mbit, oBody, oBodySz, 1 + tabcount);
        }
    }

    static int EmlPreProcess(const char* data, size_t datalen) {
        uint16_t CRLF{2573};
        size_t spos{0}, epos{0};
        char flagData[]{'D', 'A', 'T', 'A'};
        char flagDataLc[]{'d', 'a', 't', 'a'};
        char flag354[]{'3', '5', '4', ' '};
        int flagCtrl{-1};
        int flagCount{0};
        int ret{-1};

        for (size_t i = 0; i < datalen - 1; i++) {
            uint16_t* datPtr = (uint16_t*)(data+i);
            if (*datPtr == CRLF) {
                epos = i;
                if (flagCtrl == -1) {
                    if (epos - spos == 4) {
                        flagCtrl = 0;
                        for (int j = 0; j < 4; j++) {
                            if (flagData[j] != (data+spos)[j] && flagDataLc[j] != (data+spos)[j]) {
                                flagCtrl = -1;
                                break;
                            }
                        }
                    }
                } else if (flagCtrl == 0) {
                    if (epos - spos > 3) {
                        flagCtrl = 1;
                        for (int i = 0; i < 4; i++) {
                            if (flag354[i] != (data+spos)[i]) {
                                flagCtrl = 0;
                                break;
                            }
                        }
                    }
                    if (flagCtrl != 1 && ++flagCount > 5) {
                        ret = -1;
                        break;
                    }
                }

                spos = epos + 2;
                if (flagCtrl == 1) {
                    ret = (int)spos;
                    //out of range
                    if (ret >= (int)datalen) {
                        ret = -1;
                    }
                    break;
                }
            }
        }

        return ret;
    }
    //                            spos epos
    static std::vector<std::tuple<int, int, std::string>> EmlPreProcessPlus(const char* data, size_t datalen) {
        uint16_t CRLF{2573};
        //      [spos, epos)
        size_t spos{0}, epos{0};
        //smtp
        char flagData[]{'D', 'A', 'T', 'A'};
        char flagDataLc[]{'d', 'a', 't', 'a'};
        char flag354[]{'3', '5', '4', ' '};
        //pop
        char flagRetr[] {'R', 'E', 'T', 'R', ' '};
        char flagRetrLc[] {'r', 'e', 't', 'r', ' '};
        char flagOk[] {'+', 'O', 'K', ' '};
        char flagOkLc[] {'+', 'o', 'k', ' '};
        char flagOctets[] {' ', 'O', 'C', 'T', 'E', 'T', 'S'};
        char flagOctetsLc[] {' ', 'o', 'c', 't', 'e', 't', 's'};

        //imap
        std::string item;
        std::string section;

        //terminator in smtp and pop
        char flagTerm {'.'};
        //terminator in imap
        char flagTerm2 {')'};

        int flagCtrl{-1};
        int flagCount{0};
        std::vector<std::tuple<int,int, std::string>> retVec;
        //  [mailPos1, mailPos2]
        int mailPos1{-1}, mailPos2{-1};

        for (size_t i = 0; i < datalen - 1; i++) {
            uint16_t* datPtr = (uint16_t*)(data+i);
            if (*datPtr == CRLF) {
                //Delete me
//                std::string str(data + spos, i-spos);
//                cout << str << endl;

                epos = i;
                //imap
                if (flagCtrl == -4) {
                    if (epos - spos == 1) {
                        if (flagTerm2 == (data+spos)[0]) {
                            mailPos2 = spos - 3;
                            if (mailPos2 > mailPos1) retVec.push_back(std::make_tuple(mailPos1, mailPos2, "IMAP"));
                            mailPos1 = -1;
                            mailPos2 = -1;
                            //go to next part
                            flagCtrl = 20;
                        }
                    } else if (epos - spos > 1 && (data+spos)[0] == 'a' && (data+spos)[1] == '0' && epos - spos > 10) {
                        if (re2::RE2::PartialMatch(re2::StringPiece(data+spos, epos-spos), R"(a0\d+\s(OK|NO|BAD)\s(FETCH|NOOP|STORE|COPY|UID|SEARCH|EXPUNGE|CLOSE|CHECK)\s\w+)")) {
                            flagCount++;
                        }
                    }

                    if (flagCount > 2) {
                        mailPos2 = spos - 3;
                        if (mailPos2 > mailPos1) retVec.push_back(std::make_tuple(mailPos1, mailPos2, "IMAP"));

                        flagCount = 0;
                        mailPos1 = -1;
                        mailPos2 = -1;
                        flagCtrl = -1;
                    }
                }
                //pop
                /*find out terminator "\r\n.\r\n" */
                else if (flagCtrl == -3) {
                    if (epos - spos == 1) {
                        if (flagTerm == (data+spos)[0]) {
                            mailPos2 = spos - 3;
                            if (mailPos2 > mailPos1) retVec.push_back(std::make_tuple(mailPos1, mailPos2, "POP"));
                            mailPos1 = -1;
                            mailPos2 = -1;
                            //go to next part
                            flagCtrl = -1;
                        }
                    } else if (epos - spos > 6) {
                        if (re2::RE2::PartialMatch(re2::StringPiece(data+spos, epos-spos), R"(RETR\s\d+$|\+OK\s\d+\soctets$)")) {
                            if ((data+spos)[0] == 'R') {
                                flagCtrl = 10;
                            } else {
                                flagCtrl = 11;
                            }
                            flagCount++;
                        }
                    }

                    if (flagCount > 0) {
                        mailPos2 = spos - 3;
                        if (mailPos2 > mailPos1) retVec.push_back(std::make_tuple(mailPos1, mailPos2, "POP"));

                        flagCount = 0;
                        mailPos1 = -1;
                        mailPos2 = -1;
                        if (flagCtrl == -3) {
                            flagCtrl = -1;
                        }
                    }
                }
                //smtp
                /*find out terminator "\r\n.\r\n" */
                else if (flagCtrl == -2) {
                    if (epos - spos == 1) {
                        if (flagTerm == (data+spos)[0]) {
                            mailPos2 = spos - 3;
                            //avoid error due to no data
                            if (mailPos2 > mailPos1) retVec.push_back(std::make_tuple(mailPos1, mailPos2, "SMTP"));

                            mailPos1 = -1;
                            mailPos2 = -1;
                            flagCtrl = -1;
                            //just one eml in smtp session
                            break;
                        }
                    }
                }
                //initial state
                else if (flagCtrl == -1) {
                    //smtp flag
                    if (epos - spos == 4) {
                        flagCtrl = 0;
                        for (int j = 0; j < 4; j++) {
                            if (flagData[j] != (data+spos)[j] && flagDataLc[j] != (data+spos)[j]) {
                                flagCtrl = -1;
                                break;
                            }
                        }
                    }
                    //pop flag
                    else if (epos - spos > 5 && epos - spos < 10) {
                        flagCtrl = 10;
                        for (int j = 0; j < 5; j++) {
                            if (flagRetr[j] != (data+spos)[j] && flagRetrLc[j] != (data+spos)[j]) {
                                flagCtrl = -1;
                                break;
                            }
                        }

                        if (flagCtrl == 10) {
                            //RETR 123
                            for (size_t j = 5; j < epos-spos; j++) {
                                if ((data+spos)[j] < '0' || (data+spos)[j] > '9') {
                                    if (j != epos-spos -1 || (data+spos)[j] != ' ') {
                                        flagCtrl = -1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //imap flag
                    else {
                        if (re2::RE2::PartialMatch(re2::StringPiece(data+spos, epos-spos), R"(\w+\sFETCH\s\d+(\:\d+)?\s(.+)$)", &item, &section)) {
                            if (section == "RFC822") {
                                flagCtrl = 20;
                            } else if (section.find("BODY.PEEK[]") != std::string::npos) {
                                flagCtrl = 20;
                            } else if (section.find("BODY[]") != std::string::npos) {
                                flagCtrl = 20;
                            }
                        }
                    }
                }
                //smtp
                else if (flagCtrl == 0) {
                    if (epos - spos > 3) {
                        flagCtrl = 1;
                        for (int i = 0; i < 4; i++) {
                            if (flag354[i] != (data+spos)[i]) {
                                flagCtrl = 0;
                                break;
                            }
                        }
                    }
                    if (flagCtrl != 1 && ++flagCount > 5) {
                        flagCount = 0;
                        break;
                    }
                }
                //pop
                else if (flagCtrl == 10) {
                    if (epos - spos > 12) {
                        flagCtrl = 11;
                        for (int j = 0; j < 4; j++) {
                            if (flagOk[j] != (data+spos)[j] && flagOkLc[j] != (data+spos)[j]) {
                                flagCtrl = 10;
                                break;
                            }
                        }

                        if (flagCtrl == 11) {
                            for (int j = 0; j < 7; j++) {
                                if (flagOctets[7-1-j] != (data+epos-1-j)[0] && flagOctetsLc[7-1-j] != (data+epos-1-j)[0]) {
                                    flagCtrl = 10;
                                    break;
                                }
                            }
                        }
                    }

                    if (flagCtrl != 11 && ++flagCount > 2) {
                        flagCount = 0;
                        flagCtrl = -1;
                    }
                }
                //imap
                else if (flagCtrl == 20) {
                    if (epos - spos > 10) {
                        if (re2::RE2::PartialMatch(re2::StringPiece(data+spos, epos-spos), R"(\*\s\d+\sFETCH\s.+$)")) {
                            flagCtrl = 21;
                        }
                    }

                    if (flagCtrl != 21 && ++flagCount > 0) {
                        flagCount = 0;
                        flagCtrl = -1;
                    }
                }

                spos = epos + 2;

                if (flagCtrl == 1 || flagCtrl == 11 || flagCtrl == 21) {
                    mailPos1 = (int)spos;
                    //out of range
                    if (spos >= datalen) {
                        mailPos1 = -1;
                        break;
                    }

                    flagCount = 0;
                    //let it find out terminator
                    //smtp
                    if (flagCtrl == 1) {
                        flagCtrl = -2;
                    }
                    //pop
                    else if (flagCtrl == 11) {
                        flagCtrl = -3;
                    }
                    //imap
                    else if (flagCtrl == 21) {
                        flagCtrl = -4;
                    }
                }
            }
        }

        if (mailPos1 != -1) {
            if (flagCtrl == -2) {
                retVec.push_back(std::make_tuple(mailPos1, datalen - 1, "SMTP"));
            } else if (flagCtrl == -3) {
                retVec.push_back(std::make_tuple(mailPos1, datalen - 1, "POP"));
            } else if (flagCtrl == -4) {
                retVec.push_back(std::make_tuple(mailPos1, datalen - 1, "IMAP"));
            } else {
                retVec.push_back(std::make_tuple(mailPos1, datalen - 1, ""));
            }
        }
        return retVec;
    }


    //date tools
    static bool GetTody(char *s, size_t max, time_t tmstamp) {
        auto tmptr = localtime(&tmstamp);
        if (tmptr == nullptr) {
            return false;
        }

        strftime(s, max, "%Y%0m%0d", tmptr);
        return true;
    }

    static bool GetTodyDateTime(char *s, size_t max, time_t tmstamp) {
        auto tmptr = localtime(&tmstamp);
        if (tmptr == nullptr) {
            return false;
        }

        strftime(s, max, "%Y-%0m-%0d %H:%M:%S", tmptr);
        return true;
    }

    static std::tuple<bool, PrivateIPType> isPrivateIp(uint32_t ipHostEndian) {
        //取numIP的前16位
        uint32_t tag = (0xffffffff<<16) & ipHostEndian;
        if (3232235520 == tag) {
            //192 PRIVATE_IP_IN_TYPE_C
            return std::make_tuple(true, PrivateIPType::T192);
        }
        //取numIP的前12位
        tag = (0xffffffff<<20) & ipHostEndian;
        if (2886729728 == tag) {
            //172 PRIVATE_IP_IN_TYPE_B
            return std::make_tuple(true, PrivateIPType::T172);
        }
        //取numIP的前8位
        tag = (0xffffffff<<24) & ipHostEndian;
        if (167772160 == tag) {
            //10 PRIVATE_IP_IN_TYPE_A
            return std::make_tuple(true, PrivateIPType::T10);
        }
        return std::make_tuple(false, PrivateIPType::TInvalid);
    }

    static CommIPType getCommIPType(uint32_t ipNetEndian) {
        uint8_t* primacy = (uint8_t*)&ipNetEndian;
        if (*primacy <= 64) {
            return CommIPType::T1_64;
        } else if (*primacy <= 128) {
            return CommIPType::T65_128;
        } else if (*primacy <= 192) {
            return CommIPType::T129_192;
        } else {
            return CommIPType::T193_255;
        }
    }
};
#endif // EMLPARSER_HPP
