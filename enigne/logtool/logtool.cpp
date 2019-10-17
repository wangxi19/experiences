#include "logtool.h"
#include <unistd.h>
#include <stdarg.h>
#include <map>

static LogTool* logtool{nullptr};
static int logFd{-1};
static FILE* logFILE{nullptr};
static bool closeFdWhenDestroy{false};

#define BUFLEN 26
static std::map<unsigned long, char[BUFLEN]> timeFormatBuffer;

bool LogTool::SetLogFd(int fd, bool iCloseFdWhenDestroy)
{
    logFd = fd;
    closeFdWhenDestroy = iCloseFdWhenDestroy;
    try {
        logFILE = fdopen(logFd, "w");
        if (logFILE == nullptr) {
            fprintf(stderr, "%s:%s:%d Fail to call fdopen, fd %d, NULL returned.\n", __FILE__, __FUNCTION__, __LINE__);
            return false;
        }
    } catch (...) {
        fprintf(stderr, "%s:%s:%d Fail to call fdopen, fd %d. Unknown error.\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
}

LogTool *LogTool::GetInstance()
{
    if (logtool == nullptr) {
        logtool = new LogTool();
    }

    return logtool;
}

char *LogTool::GetFormatedTime(unsigned long pid)
{
    auto buf = timeFormatBuffer[pid];
    time_t timer;
    struct tm* tmInfo;

    time(&timer);
    tmInfo = localtime(&timer);
    strftime(buf, BUFLEN - 1, "%Y-%m-%d %H:%M:%S", tmInfo);

    return buf;
}

void LogTool::LOG(const char *format, ...)
{
    if (nullptr == logFILE) return;

    std::lock_guard<std::mutex> lock(mMt);
    va_list valist;
    va_start(valist, format);
    vfprintf(logFILE, format, valist);
    va_end(valist);

    return;
}

LogTool::~LogTool()
{
    if (closeFdWhenDestroy && logFd > 0) {
        close(logFd);
        logFd = -1;
    }
}

LogTool::LogTool()
{

}
