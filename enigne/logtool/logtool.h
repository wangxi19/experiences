#ifndef LOGTOOL_H
#define LOGTOOL_H

//a simplest log library
#include <time.h>
#include <stdio.h>

#ifndef LOGERROR
#define LOGERROR(format, args...) LogTool::GetInstance()->LOG("[ERROR] %s %s:%s:%d " format, LogTool::GetFormatedTime(pthread_self()), __FILE__, __FUNCTION__, __LINE__, ##args);
#endif

#ifndef LOGWARN
#define LOGWARN(format, args...) LogTool::GetInstance()->LOG("[WARN] %s %s:%s:%d " format, LogTool::GetFormatedTime(pthread_self()), __FILE__, __FUNCTION__, __LINE__, ##args);
#endif

#ifndef LOGINFO
#define LOGINFO(format, args...) LogTool::GetInstance()->LOG("[INFO] %s %s:%s:%d " format, LogTool::GetFormatedTime(pthread_self()), __FILE__, __FUNCTION__, __LINE__, ##args);
#endif

#include <string>
#include <mutex>

class LogTool
{
public:
    ~LogTool();
    static bool SetLogFd(int fd, bool iCloseFdWhenDestroy = false);
    static LogTool* GetInstance();

    //thread safe :)
    static char* GetFormatedTime(unsigned long int pid);

    //thread safe :)
    void LOG(const char* format, ...);
private:
    explicit LogTool();
    std::mutex mMt;
};

#endif // LOGTOOL_H
