#include <stdio.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

extern void doWork();

std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;

int main(int argc, char* argv[]) {
  
      doWork();
#if 0
  auto f = []{
      std::unique_lock<std::mutex> lk(m, std::defer_lock);
      lk.lock();
      cv.wait(lk, []{return ready;});
      data += " 已处理完成!";
      std::cout << data << std::endl;

      processed = true;
      lk.unlock();
      cv.notify_one();
  };
  std::thread t(f);
  
  data = "你好, 朋友"; 
  {
    std::lock_guard<std::mutex> lk(m);
    ready = true;
  }
  cv.notify_one();

  {
    std::unique_lock<std::mutex> lk(m, std::defer_lock);
    lk.lock();
    cv.wait(lk, []{return processed;});
  }

  t.join();
#endif
  return 0;
}
