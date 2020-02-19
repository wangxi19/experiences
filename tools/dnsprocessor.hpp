#ifndef DNSPROCESSOR_HPP
#define DNSPROCESSOR_HPP
#define NOLOG

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <tuple>
#include <rollBuffer.h>
#include <array>
#include <vector>
#ifndef NOLOG
#include <baselog.h>
#endif
#include <curl.hpp>
#include <iostream>
#include <iconv.h>

#define DBBUFSZ (1024*1024*10)
#define DMMAXLEN (1024)
#define IPARRAMAXLEN (20)

class DNSProcessor {
public:
    explicit DNSProcessor() {

    }

    void Init(const std::string& iESDBHostPort = "http://127.0.0.1:9200/", uint32_t size = 50000) {
        mRollBuffer.Init((int)size);
        dbBufPtr = (char*)_malloc(DBBUFSZ);
        mESDBHostPort = iESDBHostPort;
        mInited = true;
    }

    ~DNSProcessor() {
        for (const auto ptr: mallocLst) {
            if (ptr != NULL) {
                free(ptr);
            }
        }
        mallocLst.clear();
    }

    //all of these below ip addresses are using net endian
    bool addDNS(uint32_t srcip, uint32_t dstip, uint32_t date, const char* domain, std::vector<uint32_t> ipaddrs) {
        if (!mInited) {
            return false;
        }

        if (ipaddrs.size() == 0) {
            return false;
        }

        auto _ = mRollBuffer.getWriteBucket(0);
        if (_ == NULL) {
            mLosCount++;
            return false;
        }

        auto& tp = *_;
        auto& _0 = std::get<0>(tp);
        auto& _1 = std::get<1>(tp);
        auto& _2 = std::get<2>(tp);
        auto& _3 = std::get<3>(tp);
        auto& _4 = std::get<4>(tp);

        _0 = srcip;
        _1 = dstip;
        _2 = date;

        snprintf(_3, sizeof(_3), domain);
        auto iplen = ipaddrs.size() > _4.size() ? _4.size() : ipaddrs.size();
        _4.fill(0);
        for (size_t i = 0; i < iplen; i++) {
            _4[i] = ipaddrs[i];
        }

        mRollBuffer.Write();
        mInsCount++;
        return true;
    }

    void* _malloc(size_t sz) {
        void* ret = malloc(sz);
        mallocLst.push_back(ret);
        return ret;
    }
public:
    // all of these below ip addresses are using net endian
    //                  srcip       dstip   date        domain
    typedef std::tuple<uint32_t, uint32_t, uint32_t, char[DMMAXLEN], std::array<uint32_t, IPARRAMAXLEN>> DNSItem;
    CRollBuffer<DNSItem> mRollBuffer;
    char* dbBufPtr{NULL};
    std::vector<void*> mallocLst;
    std::string mESDBHostPort{"http://127.0.0.1:9200/"};
    bool mInited{false};
    uint64_t mLosCount{0};
    uint64_t mInsCount{0};
public:
    static bool TimerTaskPfn(void* arg) {
        auto tsPtr = (DNSProcessor*) arg;
        tsPtr->pfnPthread(arg);
        return true;
    }

    static void* pfnPthread(void* arg) {
        auto tsPtr = (DNSProcessor*) arg;
        if (!tsPtr->mInited) {
            return 0;
        }

        size_t offset{0};
        size_t offsetPre{0};
        char todayBuf[16]{0,};
        const char* bulkIndex = "{\"index\":{\"_index\":\"dns_yyyymmdd\"}}\n";
        const size_t bulkIndexSz = strlen(bulkIndex);
        while (true) {
            offsetPre = offset;
            auto _ = tsPtr->mRollBuffer.getReadBucket(0);
            if (_ == NULL) {
                break;
            }

            auto& tp = *_;
            if (!GetTody(todayBuf, 16, std::get<2>(tp))) {
#ifndef NOLOG
                BYBASE_LOG(ERROR, "Fail to get today, %u\n", std::get<2>(tp));
#endif
                tsPtr->mRollBuffer.Read();
                continue;
            }

            if (offset + bulkIndexSz >= DBBUFSZ) {
                if (offsetPre) {
                    SyncToDb(tsPtr->dbBufPtr, offsetPre, tsPtr->mESDBHostPort + "_bulk");
                }
                auto _pre = offsetPre;
                tsPtr->mRollBuffer.undoGetReadBucket();
                offset = 0;
                offsetPre = 0;
                if (_pre == 0) {
                    break;
                } else {
                    continue;
                }
            }

            offset += (size_t)snprintf(tsPtr->dbBufPtr+offset, (DBBUFSZ-offset), "{\"index\":{\"_index\":\"dns_%s\"}}\n", todayBuf);

            if (GetDNSItemJsonLen(tp) + offset > DBBUFSZ) {
                if (offsetPre) {
                    SyncToDb(tsPtr->dbBufPtr, offsetPre, tsPtr->mESDBHostPort + "_bulk");
                }
                auto _pre = offsetPre;
                offset = 0;
                offsetPre = 0;
                tsPtr->mRollBuffer.undoGetReadBucket();
                if (_pre == 0) {
                    break;
                } else {
                    continue;
                }
            }

            std::string doc;
            DNSItemToJson(tp, doc);
            offset += (size_t)snprintf(tsPtr->dbBufPtr+offset, (DBBUFSZ-offset), doc.c_str());

            tsPtr->mRollBuffer.Read();
        }

        if (offset > 0) {
            SyncToDb(tsPtr->dbBufPtr, offset, tsPtr->mESDBHostPort + "_bulk");
        }

        return 0;
    }

public:
    /***Tool functions***/
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

    static void SyncToDb(char* data, size_t datalen, const std::string& index) {
        CURLNS::POST(index.c_str(), data, datalen);
    }

    //{"srcip":"123.123.123.123","dstip":"321.321.321.321","date":"1577324063","date_h":"2019-12-26 09:37:28","domain":"www.baidu.com","ip_addr":["1.1.1.1","2.2.2.2"]}\n
    //{"srcip":"","dstip":"","date":"1577324063","date_h":"2019-12-26 09:37:28","domain":"","ip_addr":[]}\n

    static size_t GetDNSItemJsonLen(const DNSItem& dnsItem) {
        size_t fixedSz {100};
        size_t srcIpSz{0};
        size_t dstIpSz{0};
        size_t domainSz{0};
        size_t ipaddrSz{0};
        const auto srcIp = std::get<0>(dnsItem);
        const auto dstIp = std::get<1>(dnsItem);
        const auto& domain = std::get<3>(dnsItem);
        const auto& ipaddrs = std::get<4>(dnsItem);
        uint8_t* ptr = (uint8_t*)&srcIp;
        for (int i = 0; i < 4; i++) {
            auto _ = ptr + i;
            if (*_ < 10) {
                srcIpSz += 1;
            } else if (*_ < 100) {
                srcIpSz += 2;
            } else {
                srcIpSz += 3;
            }
        }
        srcIpSz += 3;

        ptr = (uint8_t*)&dstIp;
        for (int i = 0; i < 4; i++) {
            auto _ = ptr + i;
            if (*_ < 10) {
                dstIpSz += 1;
            } else if (*_ < 100) {
                dstIpSz += 2;
            } else {
                dstIpSz += 3;
            }
        }
        dstIpSz += 3;

        int count{0};
        for (const auto ipaddr: ipaddrs) {
            if (ipaddr == 0) {
                break;
            }
            count ++;
            ptr = (uint8_t*)&ipaddr;
            size_t tmpSz{5};
            for (int i = 0; i < 4; i++) {
                auto _ = ptr + i;
                if (*_ < 10) {
                    tmpSz += 1;
                } else if (*_ < 100) {
                    tmpSz += 2;
                } else {
                    tmpSz += 3;
                }
            }

            ipaddrSz += tmpSz;
        }

        if (count > 0) {
            ipaddrSz += (count - 1);
        }

        domainSz = quote(domain).size()-2;

        return fixedSz + srcIpSz + dstIpSz + domainSz + ipaddrSz;
    }

    static void DNSItemToJson(const DNSItem& dnsItem, std::string& oJson) {
        char buf[24]{0, };
        GetTodyDateTime(buf, sizeof(buf), std::get<2>(dnsItem));
        oJson += R"({"srcip":")";
        oJson += ip2str(std::get<0>(dnsItem));
        oJson += R"(","dstip":")";
        oJson += ip2str(std::get<1>(dnsItem));
        oJson += R"(","date":)"+std::to_string(std::get<2>(dnsItem));
        oJson += R"(,"date_h":")";
        oJson += buf;
        oJson += R"(","domain":)"+quote(std::get<3>(dnsItem));
        oJson += R"(,"ip_addr":[)";
        for (const auto ipaddr: std::get<4>(dnsItem)) {
            if (ipaddr == 0) {
                break;
            }

            oJson += R"(")";
            oJson += ip2str(ipaddr);
            oJson += R"(",)";
        }

        if (oJson[oJson.size()-1] == ',') {
            oJson[oJson.size()-1] = ']';
        } else {
            oJson += "]";
        }

        oJson += "}\n";
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

    static size_t codeConvert(char* srcCharset, char* dstCharset, char* inbuf, size_t inlen, char* outbuf, size_t outlen) {
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
};
#endif // DNSPROCESSOR_HPP
