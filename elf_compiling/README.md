# ELF的编译和运行原理


## 编译原理


### 预编译(pre-compiling)

把源代码的宏定义展开, 引入所有的头文件
```
gcc -E -o ./foo.e ./foo.c
cat ./foo.e
[IMG cat_foo.e.png]
```

### 汇编(assembly)

把源代码生成汇编代码

```
gcc -S -o ./foo.s ./foo.c
cat ./foo.s
[IMG cat_foo.s.png]
```

### 编译(compiling)

调用gun assemble 把汇编代码编译为机器码, 生成relocatable file(.o)

之所以叫relocatable file是因为它的section都还没有分配viraddr, 没有排版, 它的rel[a] section都还没有做relocate, .o 里面的符号还等待relocate

可用 `readelf -S foo.o` 查看其所有section的viraddr 都还没有分配

可用 `readelf -r foo.o` 查看其还有rel[a] section, 还没有relocate


注意 foo.c中用到了oof.c中定义的函数, 编译的时候这个函数(也叫符号 symbol, elf中变量和函数都叫symbol)还未定义, 所以会对此产生一条rel记录(`readelf -r foo.o | grep oof`),

若编译 foo.o时未使用 -fPIC参数, 那么在老版本的gcc, 这条rel记录的类型就为 **R_X86_64_PC32**, 这种类型的rel用来生成.so是不行的， 因为链接时, 对这条记录relocate是直接把绝对地址写到这个符号(oof)上, 而.so每次被加载进来的初始地址都不同, oof在.so中, 所以oof每次的地址也不同, 所以不能用绝对地址。

若编译foo.o时使用了 -fPIC参数, 这条rel记录的类型就为 **R_X86_64_PLT32**, 这种类型的符号就position independent code, 当链接时 对这条记录relocate, 会把这个符号的指向plt中的相应的记录, 跳转到plt后, 再通过plt表去寻找正在的符号地址(运行原理会详细介绍)

```
gcc -c -o foo.o ./foo.s
file ./foo.o
[IMG file_foo.o.png]
```

### linking

链接生成executable file 或者 shared library (.so)。生成.a没有做链接, 只是把一堆.o文件打成一个.a压缩包

- do relocate for all rel section

- reform rel[a].dyn and rel[a].plt section as needed

```
#gcc -shared
# or gcc -Lxxx -lxxx
```
