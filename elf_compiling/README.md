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

若编译foo.o时使用了 -fPIC参数(编译成了foo.fpic.o), 这条rel记录的类型就为 **R_X86_64_PLT32**, 这种类型的符号就position independent code, 当链接时 对这条记录relocate, 会把这个符号的指向plt中的相应的记录, 跳转到plt后, 再通过plt表去寻找正在的符号地址(运行原理会详细介绍)

![readelf_r_foo.fpic.o.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/readelf_r_foo.fpic.o.png)

```
gcc -c -o foo.o ./foo.s
file ./foo.o
[IMG file_foo.o.png]
```
![file_foo.o.png](https://github.com/wangxi19/experiences/blob/master/elf_compiling/imgs/file_foo.o.png)

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
