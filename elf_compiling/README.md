[TOC]

# ELF的编译和运行原理


## 编译原理


### 预编译(pre-compiling)

把源代码的宏定义展开, 引入所有的头文件
```
gcc -E -o ./foo.e ./foo.c
cat ./foo.e
```
![cat_foo.e.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/cat_foo.e.png)

### 汇编(assembly)

把源代码生成汇编代码

```
gcc -S -o ./foo.s ./foo.c
cat ./foo.s
```
![cat_foo.s.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/cat_foo.s.png)

### 编译(compiling)

调用gun assemble 把汇编代码编译为机器码, 生成relocatable file(.o)

之所以叫relocatable file是因为它的section都还没有分配viraddr, 没有排版, 它的rel[a] section都还没有做relocate, .o 里面的符号还等待relocate

可用 `readelf -S foo.o` 查看其所有section的viraddr 都还没有分配

![readelf_S_foo.o.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/readelf_S_foo.o.png)

可用 `readelf -r foo.o` 查看其还有rel[a] section, 还没有relocate

![readelf_r_foo.o.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/readelf_r_foo.o.png)


注意 foo.c中用到了oof.c中定义的函数, 编译的时候这个函数(也叫符号 symbol, elf中变量和函数都叫symbol)还未定义, 所以会对此产生一条rel记录(`readelf -r foo.o | grep oof`),

若编译 foo.o时未使用 -fPIC参数, 那么在老版本的gcc, 这条rel记录的类型就为 **R_X86_64_PC32**, 这种类型的rel用来生成.so是不行的， 因为链接时, 对这条记录relocate是直接把绝对地址写到这个符号(oof)上, 而.so每次被加载进来的初始地址都不同, oof在.so中, 所以oof每次的地址也不同, 所以不能用绝对地址。

若编译foo.o时使用了 -fPIC参数(编译成了foo.fpic.o), 这条rel记录的类型就为 **R_X86_64_PLT32**, 这种类型的符号就position independent code, 当链接时 对这条记录relocate, 会把这个符号的指向plt中的相应的记录, 跳转到plt后, 再通过plt表去寻找正确的符号地址(运行原理会详细介绍)

![readelf_r_foo.fpic.o.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/readelf_r_foo.fpic.o.png)

```
gcc -c -o foo.o ./foo.s
file ./foo.o
[IMG file_foo.o.png]
```
![file_foo.o.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/file_foo.o.png)

--- 

- **fPIC用法**

程序编译最终生成两种类型文件, .so 或 executable 文件, 这两种文件又都是由 .o文件ld链接生成。

fPIC的使用只需在编译生成.o阶段。 

.o对应一个.c文件, 若这个.c文件中使用到了未定义的符号(符号是指变量或函数, 未定义是指在当前.c文件及其所包含的所有头文件中未定义, 因为编译生成.o文件时, 编译器的视界只看到当前.c文件及其所包含的所有头文件{头文件实际上在预编译的时候把所有内容都写到了.c文件中}, 看不到其它.c文件, 看到其它.c文件{这里的其它.c文件是指.c生成的.o文件 或 .c生成的.so文件}是在ld链接的步骤).

若.c文件使用了未定义的符号, 此时生成.o 文件会针对这些未定义的符号创建 rel[a].text section 条目, 一个符号创建一条, 表示这些 .o文件在后续ld生成.so 或 executable文件时, 需要对其relocate. 若生成.o时未带参数 `-fPIC`, 则未定义的符号在 rel[a].text section中的条目, rel type就为**R_X86_64_PC32**, 带了 `-fPIC` 参数rel type 为**R_X86_64_PLT32**。

**R_X86_64_PC32**类型的.o文件是不能ld为.so文件的, 不管**R_X86_64_PC32**对应的这个符号是属于此.so中(用于生成此.so的其它.o文件中)还是属于其它的.so中, 这是因为这种 rel type使用的是绝对地址, 若符号属于此.so中, 对于.so 每次被加载进内存的地址都不相同, 所以也无法在ld生成.so时relocate, 因为程序未运行 无法固定符号的viraddr. 若符号属于其它.so中一样的, 在ld时无法固定符号的viraddr, 所以不能relocate **R_X86_64_PC32**。所以需要`-fPIC` 参数rel type 为**R_X86_64_PLT32**, 在 ld生成.so时, relocate .o的**R_X86_64_PLT32**, 虽然此时也无法固定符号的viraddr, 但是会为此符号再重新生成一条 rel type 为 **R_X86_64_JUMP_SLO** 的 rel[a].plt section条目在.so中, 移除原来的**R_X86_64_PLT32**条目, 生成的.so中符号被最终定位到.got表的对应条目中, 在程序完全加载到内存中后(完全加载是指exectable文件和它依赖的所有.so文件都被加载到内存中, 分配好了内存位置), 根据LD_BIND_NOW的设置来立刻relocate或者lazy relocate **R_X86_64_JUMP_SLO**

但**R_X86_64_PC32**类型的.o文件是能ld为executable文件的.不管**R_X86_64_PC32**对应的这个符号是属于此 executable文件(用于生成此exe文件的其它.o文件)还是属于其它的.so中。
若符号属于此executable文件, 则在ld时就能获取符号的绝对viraddr, 能对 **R_X86_64_PC32** relocate, 生成的exe文件就不会再对此符号重新创建 **R_X86_64_JUMP_SLO** rel条目了, 因为此符号已经reloccate了。
若符号属于其它.so文件, ld做法与生成.so相同, 为**R_X86_64_PC32**的符号重新创建一条**R_X86_64_JUMP_SLO**的条目。

现在的linux都支持ASLR(address space layout randomization), 导致executable文件加载到内存运行时, 每次它的起始地址也随机。但是原则上executable每个segment加载到内存的viraddr都会按照segment定义的地址来。操作系统层面的ASLR不影响我们把exectable理解为地址固定(不像.so每次加载进来的起始位置是真随机, 所以.so里面的符号viraddr在ld阶段是拿不到的), 所以executable中的所有符号的viraddr在ld的阶段时就能够固定了(能计算出来), 能拿到每个符号的viraddr。

关闭ASLR `echo 0 > /proc/sys/kernel/randomize_va_space`


### 链接(linking)

调用ld, 链接生成executable file 或者 shared library (.so)。生成.a没有做链接, 只是把一堆.o文件打成一个.a压缩包

不管是生成exe或.so文件, 首先会对.o文件进行relocate

把.o的所有 rel[a] section relocated。

若生成exe文件, .o中有用到其它库的符号, 会重新为exe生成 rel[a].dyn 和 rel[a].plt section, rel[a].dyn用来relocate那些外部的变量, 会在程序一开始加载进内存, ld就会对其进行relocate(因为程序加载进内存后, 包括所有的第三方库.so也加载进了内存, 那么那些.so的地址也就确定了, .so中的符号的地址也就确定了, 所以此时可做relocate), 

然后再把控制权转交给程序。而 rel[a].plt 用来relocate那些使用到的外部函数, 之所以和rel[a].dyn分开是因为若不设置 LD_BIND_NOW 环境变量的话, rel[a].plt中的条目采用lazy relocate的方
式, 只有第一次调用这个函数的时候才会进行relocate, 设置了 LD_BIND_NOW(LD_BIND_NOW不为空), 那么rel[a].plt也和rel[a].dyn一样, 程序一开始加载进内存后, 就会relocate

![readelf_r_main.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/readelf_r_main.png)

若生成.so文件, 与exe相同, .so中若有用到其它库的符号, 会重新为.so生成 rel[a].dyn 和 rel[a].plt section, 特别注意的是即使.so中没有用到其它库的符号, 用到了自己定义的函数(不在同一个.o中, foo.c中用到了oof.c中的oof函数), 也会重新生成rel[a].plt section, 因为.so每次加载进来的初始地址不同, 所以里面的函数每次的地址都不同, 无法使用绝对地址

![readelf_r_libfoof.so.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/readelf_r_libfoof.so.png)

关于dynamic相关的可查看 `readelf -d xxx`


```
#gcc -shared
# or gcc -Lxxx -lxxx
```

## 运行原理

### 加载程序及动态链接

- 创建进程：
  内核会为可执行文件创建一个新的进程，这个进程将成为程序的执行实例。
  
- 分配内存：
  内核为进程分配一块内存区域，用于存储可执行文件的代码段（text segment）、数据段（data segment）、堆（heap）和栈（stack）。
  
- 装载ELF文件：
  内核会将可执行文件的内容从磁盘加载到分配的内存区域中。这包括将代码段加载到可执行内存、数据段加载到可写内存以及其他必要的初始化工作。

- 动态链接so
1. 查找依赖库：操作系统会查找可执行文件所需的so, 可用`ldd`或`readelf -d`命令查看依赖的so, 通常从标准的so路径查找, 若定义了**LD_LIBRARY_PATH**环境变量, 也会从定义的路径中查找。
2. 加载共享库：找到依赖库后，它们会被加载到进程的地址空间中。每个库都会有一个独立的内存映射, 可查看 **/proc/pid/maps** 确认, 若定义了**LD_PRELOAD**环境变量, 定义的so也会被加载到进程的内存中。
3. 解析符号：主程序和所有so中未定义的变量(使用的其它so中定义的变量, so也会依赖其它so, 但所有依赖的so都会被加载到内存中), 在so加载到内存中时, 会解析这些变量, 赋上对应值; 而主程序和所有so中未定义的函数(使用的其它so中定义的函数), 默认行为是在第一次执行的时候才resolve这个函数(从其它so中找到实现), 若**LD_BIND_NOW**环境变量设为1, 函数的解析也会和变量一样, 在so加载进来后就执行解析。若**LD_PRELOAD**有定义, 其定义的so有最高优先级, 主程序和所有so中未定义的变量、函数都会优先使用**LD_PRELOAD**中的定义，这样就能注入函数(如 自己定义read write函数, 使用**LD_PRELOAD**注入)

### 反汇编查看磁盘上的main运行逻辑

使用 `objdump -D ./main` 查看, 注意之前所有编译加上 **-g3 -O0**参数, -O0让代码不被优化, 看到原始逻辑, -g3生成额外的debug相关section, 方便gdb查看

查看main中调用foo函数
![objdump_main_1.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_1.png)

跳转到plt表中 foo@plt
![objdump_main_2.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_2.png)

再从plt表跳到got中的对应记录, 所有的Position independent code 都使用got表做跳转
![objdump_main_3.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_3.png)

got中再跳回到foo@plt, 再跳到plt的第一条记录
![objdump_main_4.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_4.png)

再跳到got中的对应记录, 此时还未relocate, 为0. 见红色
got中保存着符号的地址
![objdump_main_5.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_5.png)


而gvar是存在于.bss section中(未初始化的变量存在于.bss中), 未relocate, 对应地址空间上全为0

![objdump_main_gvar_1.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_gvar_1.png)


![objdump_main_gvar_2.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/objdump_main_gvar_2.png)


### gdb查看加载到内存中的main运行逻辑

使用 `gdb main` 查看.

一开始调用foo

![gdb_main_1.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/gdb_main_1.png)

反汇编对应地址 (foo@plt), 与磁盘上main objdump查看一致
![gdb_main_2.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/gdb_main_2.png)

查看对应got表中的记录, 发现其指回foo@plt(400516), 再跳转到plt表(4004f0)
![gdb_main_3.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/gdb_main_3.png)

再从plt表跳转到对应got中的记录601010(此时就与磁盘上的main有区别了), 记录的地址指向 dl_runtime_resolve, dl运行relocate
![gdb_main_4.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/gdb_main_4.png)

运行一次之后, foo这条 rela.plt已经被relocate了

![gdb_main_5.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/gdb_main_5.png)

若在gdb中 `set environment LD_BIND_NOW=1`, 则foo@plt所指向的got中的记录在程序一开始加载到内存中时, 就会relocate


而gvar在程序加载到内存中时, 就relocate。也就是r 后, 到main之前。如图 gvar已经被relocate, 值为1024。具体的relocate算法根据rel的类型不同而不同, 此处gvar的rel类型为 R_X86_64_COPY,即把gvar的值从so中拷贝到main里的gvar
![gdb_main_gvar_1.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/gdb_main_gvar_1.png)

### 程序运行时, 栈的使用

程序运行时, 栈的地址空间是从高地址到低地址
程序运行时, 从.text section, 也就是代码区一条一条读取指令, CPU执行指令。一些与栈相关的指令会操作栈, push入栈操作, 将值写到栈上。执行函数调用指令call时, CPU也会隐式的进行入栈操作, 将当前执行的指令地址入栈(retaddr), 以便call 函数调用完毕后, 能跳转回之前执行的位置继续执行

寄存器 rbp保存当前函数的栈底地址, rsp保存当前函数的栈顶地址

拿 main举例, 当执行到 oof函数中时, 堆栈如图:


![stack.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/stack.png)

```
--- main --- 0x00400620
[rbp - 0x00] <- 0x7fffffffda50
[argv]
[argc]
--- foo --- 0x7ffff7bd9657
[retaddr - 0x00400620] <- 0x7fffffffda40+8
[rbp - 0x7fffffffda50] <- 0x7fffffffda40
[int b]
[int a]
--- oof ---
[retaddr - 0x7ffff7bd9657] <- 0x7fffffffda20 + 8
[rbp - 0x7fffffffda40] <- 0x7fffffffda20
[int b]
[int a]
[rsp]
```

disassemble 查看, 每个函数的前两条汇编指令都为 

`push   %rbp`, 把当前的栈底地址入栈. 这就是栈空间上 [rbp] 的来源
`mov    %rsp,%rbp`, 把当前的栈顶地址作为栈底地址

若gcc编译时, 带有 -fomit-frame-pointer 参数, 则不会使用rbp, 完全用rsp+偏移来定位堆栈。函数中也没有 `push %rbp` 和 `mov    %rsp,%rbp`两条指令了

rbp、rsp、retaddr(retaddr保存着caller指令的地址, callee执行完后会跳到retaddr的地址执行对应指令)都为指针, 保存着对应的地址, 指针sizeof 为8个字节。
当然熟悉了栈的内存分布以后, 从oof中找到rbp的内存位置, 可直接 ((long long*)&b) + 1, 因为它就在b之前


程序的栈结构如下
```
direction of     |                                 |
  growth of      +---------------------------------+ 
   stack         | Parameters passed by fn1(caller)|
from higher addr.|                                 |
to lower addr.   | Direction of growth is opposite |
      |          |   to direction of stack growth  |
      |          +---------------------------------+ <-- SP on entry to fn2
      |          | Return address from fn2(callee) | <-- 保存着返回的地址, fn2执行完后, 返回到该地址上执行该地址上的指令
      V          +---------------------------------+ 
                 | Callee saved registers being    | 
                 |   used in the callee function   | <-- 目前就保存了rbp, 即上一个栈的栈底位置
                 +---------------------------------+
                 | Local variables of fn2          |
                 |(Direction of growth of frame is |
                 | same as direction of growth of  |
                 |            stack)               |
                 +---------------------------------+ 
                 | Arguments to functions called   |
                 | by fn2                          |
                 +---------------------------------+ <- Current SP after stack 
                                                        frame is allocated
```
reference https://stackoverflow.com/questions/1677415/does-stack-grow-upward-or-downward 
Ganesh Gopalasubramanian
