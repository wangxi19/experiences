#include <stdio.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <string.h>
#include <iostream>

static void clearScreen() {
  // CSI[2J clears screen, CSI[H moves the cursor to top-left corner
  std::cout << "\x1B[2J\x1B[H" << std::flush;
}

void task_atomic() {
    std::atomic<int> aInt; 
    int cnt = 1000000;
    
    std::thread t1([cnt, &aInt]{
        int i = 0;
        for (; i < cnt; i++) {
          aInt++;
        }
        printf("==> %d\n", i);
    });

    std::thread t2([cnt, &aInt]{
        int i = 0;
        for (; i < cnt; i++) {
          aInt++;
        }
        printf("==> %d\n", i);
    });
    
    t1.join();
    t2.join();

    printf("task_atomic: %d\n", aInt.fetch_add(0, std::memory_order_relaxed));
    return;
}

void task_readfile() {
  const char* iPath = "/tmp/hello.txt";
  std::fstream f(iPath, std::ios_base::in);
  if (!f.is_open()) {
    fprintf(stdout, "failed to open %s", *iPath);
    return;
  }

  char buffer[1024]{'\0'};
  while (!f.eof() /*&& EOF != f.peek()*/) {  
    f.get(buffer, 1024, '\n');
    fprintf(stdout, "%s", buffer); 
    memset(buffer, '\0', 1024);
    
    //jump to the next available character(skip '\n') 
    if ('\n' == f.peek()) {
      char linebreak = f.get();
      fprintf(stdout, "%c", linebreak);
    }
    //fprintf(stdout, "position: %d\n", f.tellg());

  }
}

void task_flush() {
  clearScreen();
  std::cout << "Loading" << std::flush;
  int cnt{0};
  while (cnt++ < 30) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (cnt % 3 == 0) {
      clearScreen();
      std::cout << "Loading" << std::flush;
    }
    std::cout << "." << std::flush;
  }
  clearScreen();
  std::cout << "Hello world" << std::endl;
}

void doWork() {
//    task_atomic();
    task_flush();
    return;
}

