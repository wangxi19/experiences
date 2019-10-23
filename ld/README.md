假设有一个静态库 libtool.a, 稍后把 libtool.a链接到main.o上生成一个可执行文件 main,
你会发现 用`nm libtool.a` 看里面的符号并不是所有都存在于 main中, 只有main.cpp用到的符号才存在于main中。
这样导致一个问题,假设在main.cpp中用dlopen打开一个so, 这个so用到了 libtool.a 中其它的符号(main.cpp中未使用的符号, 也就未连接到main中), 
那么就找不到这个符号.
需要使用连接参数 控制 把tool.a所有的符号(不管main.cpp用到没用到) 都连接进main 
```shell
g++ -o main main.o -Wl,-E -Wl,--whole-archive -l:libtool.a -Wl,--no-whole-archive -lpthread
```
使用 -Wl,--whole-archive 参数控制, 注意 链接完需要的库后, 用 -Wl,--no-whole-archive 关闭这个选项, 避免后续库也受影响 而产生其它错误(如 multiple definition)

`-Wl,-E` 是导出所有符号, 这样dlopen打开的so里面 对这些符号才有可见性
