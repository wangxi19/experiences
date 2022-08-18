[TOC]

### compiling linux

### gcc g++

1. Statically link(ld params: -static, -Bstatic)

The -Bstatic and -static options are currently used, and are not the same:
-static means: perform a completely static link (no shared libraries used at all).
-Bstatic means: for any -lfoo that follows, use only archive version of the library.

2. gcc pass params to linker(ld)

-Wl,option
Pass option as an option to the linker.  If option contains commas, it is split into multiple options at the commas

so gcc statically link: gcc -Wl,-static or gcc -Wl,-Bstatic

 3. address sanitizer
 
 enable address sanitizer (for detecting buffer overflow, used after free ...)
 ```bash
 #install the address sanitizer package
 yum install libasan.x86_64
 # enable address sanitize in gcc
 gcc -fsanitize=address -g3 -O0 -o junk ./junk.c
 ```
 just normally run the program, after buffer overflow or used memory after free ..., the program will exit with detail memory error message( ./junk 2>./mem_err.log)
 
### Writing Efficient Code (C, C++ family)
 
#### Memory
 
1. Memory Copy
 
Don't use libc function memcpy, strcpy
 
2. Memory Allocation
 
Don't allocate memory from system frequently, use memory pool to instead malloc.
 
3. Consider memory alignment, NUMA wareness: per-lcore cache, per numa node using local memory
 
4. Concurrent access to the same memory area(read-write): use per lcore variable.
 
5. Distribution memory operation across memory channels
 
6. Locking memory pages
 
The underlying operating system is allowed to load/unload memory pages at its own discretion. These page loads could impact the performance,
as the process is on hold when the kernel fetches them.
To avoid thess, could pre-load, and lock the into memory with the `mlockall()` call.
```c
if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
   fprintf(stderr, "mlockall() failed\n");
   exit(1);
}
```
 
7. On x86, atomic operation imply a lock prefix before the instruction. avoid lock situation, using per core variable instead. using RCU(Read-Copy-Update) algorithm instead simple rwlocks.
 
8. Use c11 atomic builtins in aarch64
 
9. Inline function
