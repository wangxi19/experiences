#include <iostream>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <fcntl.h>
#include <map>
#include <list>

#define MAX_EVENTS 1024

#define LOGERROR(fmt, args...) \
    fprintf(stderr,"%s %s:%d " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##args);

using namespace std;

struct smart_ptr
{
    explicit smart_ptr() {

    }
    smart_ptr(void* ptr) {
        this->m_ptr = ptr;
    }

    void* operator* () {
        return this->m_ptr;
    }

//    smart_ptr& operator =(smart_ptr& other) {
//        this->m_ptr = other.m_ptr;
//        return *this;
//    }

//    smart_ptr& operator= (void* ptr) {
//        this->m_ptr = ptr;
//        return *this;
//    }

    ~smart_ptr() {
        if (m_ptr != nullptr) {
            free(m_ptr);
        }
    }

    void * m_ptr{nullptr};
};

//101.200.0.0/15
bool iprange(std::string ip, uint32_t& oStart, uint32_t& oEnd) {
    in_addr addr;
    size_t pos = ip.find('/');
    if (pos == std::string::npos || pos == ip.size() - 1) {
        if (pos == ip.size() - 1) {
            ip = ip.substr(0, pos);
        }

        if (0 == inet_aton(ip.c_str(), &addr)) {
            LOGERROR("")
            return false;
        }

        oStart = addr.s_addr;
        oEnd = addr.s_addr;
        return true;
    }

    auto realIp = ip.substr(0, pos);
    auto postfix = ip.substr(pos + 1);
    auto postfixInt = std::atoi(postfix.c_str());

    if (0 == inet_aton(realIp.c_str(), &addr)) {
        LOGERROR("")
        return false;
    }

    uint32_t ipaddr = ntohl(addr.s_addr);
    //    auto ipaddrptr = &ipaddr;
    uint32_t pad{0xFFFFFFFF};
    //    auto padptr = &pad;
    pad = pad >> postfixInt;

    oStart = htonl(ipaddr + 1);

    ipaddr |= pad;

    oEnd = htonl(ipaddr - 1);

//    addr.s_addr = oStart;
//    std::cout << inet_ntoa(addr) << " ";
//    addr.s_addr = oEnd;
//    std::cout << inet_ntoa(addr) << std::endl;

    return true;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        _exit(1);
    }

    uint32_t start{0}, end{0};
    iprange(argv[1], start, end);

    int arg{0};
    int sock{-1};
    smart_ptr ptr = malloc(1024);
    memset(*ptr, 0, 1024);
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);

    in_addr addrin;

    struct epoll_event ev, events[MAX_EVENTS];

    auto epollfd = epoll_create(MAX_EVENTS);
    if (epollfd == -1) {
        LOGERROR("epoll_create");
        exit(EXIT_FAILURE);
    }

    int count{0};
    std::map<int, uint32_t> fdIpMap;
    std::list<int> sockLst;

    for (uint32_t i = ntohl(start); i <= ntohl(end); i++) {
        addrin.s_addr = htonl(i);
        sin.sin_addr = addrin;

        sock = socket(AF_INET, SOCK_STREAM, 0);

        if (sock <= 0) {
            LOGERROR("Fail to call socket, sock %d, count %d", sock, count);
            continue;
        }

        arg = fcntl(sock, F_GETFL, NULL);
        if (arg < 0) {
            LOGERROR("sock %d, count %d", sock, count)
            close(sock);
            continue;
        }

        arg |= O_NONBLOCK;
        if (fcntl(sock, F_SETFL, arg) < 0) {
            LOGERROR("")
            close(sock);
            continue;
        }

        if (0 != connect(sock, (struct sockaddr *)&sin, sizeof(sin))) {
            if (errno != EINPROGRESS) {
                LOGERROR("")
                close(sock);
                continue;
            }
        }

        ev.events = EPOLLIN;
        ev.data.fd = sock;

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
            LOGERROR("epoll_ctl: listen_sock");
            close(sock);
            continue;
        }

        //Don't care efficiency, because it just is a little tool
        fdIpMap.insert(std::make_pair(sock, (uint32_t)(addrin.s_addr)));
        sockLst.push_back(sock);

        count++;
        if (count == 1000) {
            for(;;) {
                auto nfds = epoll_wait(epollfd, events, MAX_EVENTS, std::atoi(argv[2]));
                if (nfds == -1) {
                    //clear sock fd caches
                    LOGERROR("epoll_pwait errorno: %d", errno);
                    for (const auto& sockFd: sockLst) {
                        ev.events = EPOLLIN;
                        ev.data.fd = sockFd;
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockFd, &ev);
                        close(sockFd);
                    }
                    sockLst.clear();
                    fdIpMap.clear();
                    count = 0;
                    break;
                }

                if (nfds == 0) {
                    //clear sock fd caches
                    for (const auto& sockFd: sockLst) {
                        ev.events = EPOLLIN;
                        ev.data.fd = sockFd;
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockFd, &ev);
                        close(sockFd);
                    }
                    sockLst.clear();
                    fdIpMap.clear();
                    count = 0;
                    break;
                }

//                count -= nfds;

                for (int n = 0; n < nfds; ++n) {
                    ssize_t rsz = read(events[n].data.fd, *ptr, 1024);
                    if (rsz > 0) {
                        std::string respStr((char*) *ptr, rsz);
                        while (true) {
                            if (respStr.size() >0 && (*(respStr.end()-1) == '\n' || *(respStr.end()-1) == '\r')) {
                                respStr.assign((char*) *ptr, respStr.size()-1);
                                continue;
                            }

                            break;
                        }
                        std::string device;
                        auto pos = respStr.find_first_of(" ");
                        if (std::string::npos != pos && pos != respStr.size()-1) {
                            device = respStr.substr(pos+1);
                        }
                        std::cout << respStr << "\t" << inet_ntoa((*(in_addr*)(&fdIpMap[events[n].data.fd]))) << " " << device << std::endl;
                    }
                    memset(*ptr, 0, 1024);

                    ev.events = EPOLLIN;
                    ev.data.fd = events[n].data.fd;
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1) {
                        LOGERROR("epoll_ctl: listen_sock");
                        close(events[n].data.fd);
                        sockLst.remove(events[n].data.fd);
                        fdIpMap.erase(events[n].data.fd);
                        continue;
                    }

                    close(events[n].data.fd);
                    sockLst.remove(events[n].data.fd);
                    fdIpMap.erase(events[n].data.fd);
                }
            }
        }


//        send(sock, "SSH-2.0-OpenSSH_7.9\r\n", sizeof("SSH-2.0-OpenSSH_7.9\r\n"), 0);
    }

    if (count > 0) {
        for(;;) {
            auto nfds = epoll_wait(epollfd, events, MAX_EVENTS, std::atoi(argv[2]));
            if (nfds == -1) {
                //clear sock fd caches
                LOGERROR("epoll_pwait errorno: %d", errno);
                for (const auto& sockFd: sockLst) {
                    ev.events = EPOLLIN;
                    ev.data.fd = sockFd;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockFd, &ev);
                    close(sockFd);
                }
                sockLst.clear();
                fdIpMap.clear();
                count = 0;
                break;
            }

            if (nfds == 0) {
                //clear sock fd caches
                for (const auto& sockFd: sockLst) {
                    ev.events = EPOLLIN;
                    ev.data.fd = sockFd;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockFd, &ev);
                    close(sockFd);
                }
                sockLst.clear();
                fdIpMap.clear();
                count = 0;
                break;
            }

//                count -= nfds;

            for (int n = 0; n < nfds; ++n) {
                ssize_t rsz = read(events[n].data.fd, *ptr, 1024);
                if (rsz > 0) {
                    std::string respStr((char*) *ptr, rsz);
                    while (true) {
                        if (respStr.size() >0 && (*(respStr.end()-1) == '\n' || *(respStr.end()-1) == '\r')) {
                            respStr.assign((char*) *ptr, respStr.size()-1);
                            continue;
                        }

                        break;
                    }

                    std::string device;
                    auto pos = respStr.find_first_of(" ");
                    if (std::string::npos != pos && pos != respStr.size()-1) {
                        device = respStr.substr(pos+1);
                    }
                    std::cout << respStr << "\t" << inet_ntoa((*(in_addr*)(&fdIpMap[events[n].data.fd]))) << " " << device << std::endl;
                }
                memset(*ptr, 0, 1024);

                ev.events = EPOLLIN;
                ev.data.fd = events[n].data.fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1) {
                    LOGERROR("epoll_ctl: listen_sock");
                    close(events[n].data.fd);
                    sockLst.remove(events[n].data.fd);
                    fdIpMap.erase(events[n].data.fd);
                    continue;
                }

                close(events[n].data.fd);
                sockLst.remove(events[n].data.fd);
                fdIpMap.erase(events[n].data.fd);
            }
        }
    }

    return 0;
}
