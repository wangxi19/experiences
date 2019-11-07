#include <stdio.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <execinfo.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <limits.h>

int ggetBtDepth{20};
std::vector<std::tuple<unsigned long int, unsigned long int, std::string>> gSpams;
static std::vector<std::string> Split(const std::string &s, const std::string &c, bool skipEmptyPart, int maxCnt)
{
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

extern "C" int getbt0(std::vector<unsigned long int>& snms) {
    void* arr[ggetBtDepth];
    size_t size;
    size = backtrace(arr, ggetBtDepth);
    for (size_t i = 0; i < size; i++) {
        auto r = (unsigned long int)arr[i];
        snms.push_back(r);
    }

    return snms.size() > 0 ? 0 : -1;
}

extern "C" int getbt1(std::vector<std::string>& snms) {
    void* arr[ggetBtDepth];
    size_t size;
    size = backtrace(arr, ggetBtDepth);

    for (size_t i = 0; i < size; i++) {
        auto r = (unsigned long int)arr[i];
        for (const auto& tp: gSpams) {
            if (std::get<0>(tp) <= r && r <= std::get<1>(tp)) {
                snms.push_back(std::get<2>(tp));
                break;
            }
        }
    }

    return (size > 0) ? 0 : -1;
}

extern "C" int getbt2(std::vector<std::string>& snms) {
    void* arr[ggetBtDepth];
    size_t size;
    size = backtrace(arr, ggetBtDepth);

    auto ret = backtrace_symbols(arr, size);
    for (size_t i = 0; i < size; i++) {
        snms.push_back(ret[i]);
    }

    return (size > 0) ? 0 : -1;
}

extern "C" int getbt3(int fd) {
    if (fd < 0) {
        return 1;
    }
    void* arr[ggetBtDepth];
    size_t size;
    size = backtrace(arr, ggetBtDepth);

    backtrace_symbols_fd(arr, size, fd);

    return 0;
}


extern "C" int getspam () {
    char fpath[1024]{0,};
    int now = time(NULL);
    long long int rd = random()*random();
    snprintf(fpath, 1023, "%s", R"(`echo -e '\0160\0143'|rev`)");
    snprintf(fpath + strlen(fpath), 1023 - strlen(fpath), " \"%s", R"(`echo -e '/\0143\0157\0162\0160/'|rev`)");
    snprintf(fpath + strlen(fpath), 1023 - strlen(fpath), "%d", getpid());
    snprintf(fpath + strlen(fpath), 1023 - strlen(fpath), "%s\"", R"(`echo -e '\0163\0160\0141\0155/'|rev`)");
    snprintf(fpath + strlen(fpath), 1023 - strlen(fpath), " /tmp/.%d.%lld.spam", now, rd);
    system(fpath);
    snprintf(fpath, 1023, "/tmp/.%d.%lld.spam", now, rd);
    struct stat s;
    stat(fpath, &s);
    if (s.st_size == 0) {
        return -1;
    }
    auto fd = open(fpath, O_RDONLY);
    if (fd <= 0) {
        return -1;
    }
    auto mps = (char*)mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close (fd);
    snprintf(fpath, 1023, "rm -f /tmp/.%d.%lld.spam", now, rd);
    system(fpath);
    if (!mps) {
        return -2;
    }

    {
        std::string mpstr(mps, s.st_size);
        munmap(mps, s.st_size);
        auto lines = Split(mpstr, "\n", true, -1);
        for (const auto& line: lines) {
//            if (line.find(".so") == std::string::npos) {
//                continue;
//            }

            auto chars = Split(line, " ", true, -1);
            if (chars.size() != 5 && chars.size() != 6) {
                continue;
            }

            auto range = chars[0];
            std::string snm;
            if (chars.size() == 6) {
                snm = chars[chars.size()-1];
            }

            auto rangevec = Split(range, "-", true, -1);
            if (rangevec.size() != 2) {
                continue;
            }

//            if (snm.find(".so") == std::string::npos) {
//                continue;
//            }

//            if (snm.find("/") != std::string::npos && snm.find("/") != snm.size() - 1) {
//                snm = snm.substr(snm.find_last_of("/") + 1, snm.size() - snm.find_last_of("/") + 1);
//            }

            char* end;
            auto r0 = strtoul(rangevec[0].c_str(), &end, 16);
            if (r0 == 0 && end == rangevec[0].c_str()) {
                /*str was not a number*/
                continue;
            } else if (r0 == ULLONG_MAX && errno) {
                /*the value of str does not fit in unsigned long long*/
                continue;
            } else if (*end) {
                /*str began with a number but has junk left over at the end*/
                continue;
            }

            auto r1 = strtoul(rangevec[1].c_str(), &end, 16);
            if (r1 == 0 && end == rangevec[1].c_str()) {
                /*str was not a number*/
                continue;
            } else if (r1 == ULLONG_MAX && errno) {
                /*the value of str does not fit in unsigned long long*/
                continue;
            } else if (*end) {
                /*str began with a number but has junk left over at the end*/
                continue;
            }

            gSpams.push_back(std::make_tuple(r0, r1, snm));
        }
    }

    return 0;
}
