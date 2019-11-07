# a lib for getting program backtrace


# Using condition

when program is crashed, you can use the lib to trace stacks, and troubleshoot which ".so"s cause the crash


# Example

```c++
//main.cpp 
#include <signal.h>
using namespace std;
extern "C" int getbt1(std::vector<std::string>& snms);
extern "C" int getspam();

void handler(int sig) {

   std::vector<std::string> vec;
   getbt1(vec);
   for (const auto& v: vec) {
      cout << v << endl;
   }
}


int main () {
  signal(SIGSEGV, handler);

  /*must call the function befor program crash*/
  /*and after all ".so" has been loaded*
  getspam();

  std::vector<int> a;
  a[100]++;

  return 0;
}
```
