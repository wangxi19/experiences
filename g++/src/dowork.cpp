#include <stdio.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

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

void* myThreadFun(void *vargp) {
  (void*)vargp;
  fprintf(stdout, "Thread is working\n");
  return NULL;
}

void task_c_thread() {
  pthread_t t;
  pthread_create(&t, NULL, myThreadFun, NULL);  

  pthread_join(t, NULL);
  fprintf(stdout, "[result]: \n");
}

void task_c_fork() {
    fprintf(stdout, "Before fork\n");
    fork();
    fprintf(stdout, "After fork\n");
    myThreadFun(NULL);
}

int task_c_file() {
    FILE* fp = fopen("/root/Top24Million-WPA-probable-v2.txt", "r");
    if (!fp) {
        fprintf(stderr, "File opening failed\n");
        return EXIT_FAILURE;
    }
    
    char buf[1024];
    memset((void*)buf, '\0', 1024);
    int cnt = 10;
    while (fgets(buf, sizeof buf, fp) != NULL && cnt-- > 0)
     fprintf(stdout, "%s", buf); 

    fclose(fp);
    return 0;
}

void task_wchar_to_char() {
    char* str = new char[1024]{'\0'};
    wchar_t array[] = L"Hello World";
    wcstombs(str, array, 1024);
    fprintf(stdout, "task_wchar_to_char: %s\n", str);
    delete str;
}

void doWork() {
//    task_atomic();
//    task_flush();
//    task_c_thread();
//    task_c_fork();
//    task_c_file();
    task_wchar_to_char();
    return;
}

