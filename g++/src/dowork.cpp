#include <stdio.h>
#include <atomic>
#include <thread>
#include <chrono>


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

void doWork() {
    task_atomic();
    return;
}

