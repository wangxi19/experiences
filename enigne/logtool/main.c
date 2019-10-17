#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "logtool.h"
#include <thread>
#include <pthread.h>

void print(int flag) {
    for (int i = 0; i < 1000; i++) {
        if (flag == 1) {
            LOGERROR("thread id: %d\n", pthread_self());
        } else if (flag == 2) {
            LOGWARN("thread id: %d\n", pthread_self());
        } else {
            LOGINFO("thread id: %d\n", pthread_self());
        }
    }
}

int main()
{
  auto fd = open("/tmp/aaa", O_RDWR | O_CREAT | O_APPEND);
  LogTool::SetLogFd(fd);

  std::thread t1(print, 1);
  std::thread t2(print, 2);
  std::thread t3(print, 3);
  t1.join();
  t2.join();
  t3.join();
  return 0;
}
