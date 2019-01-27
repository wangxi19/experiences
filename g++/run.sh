#!/bin/bash
test -d ./lib
if [ 1 -eq $? ]
then
    mkdir ./lib && mkdir ./lib/shared && mkdir ./lib/static
fi

if [ "-shared" = "$1" ]
then
    g++ -std=c++11 -fPIC -c -o ./lib/shared/dowork.o ./src/dowork.cpp 
    g++ -shared -fPIC -Wl,-soname,libdowork.so.1 -o ./lib/shared/libdowork.so.1.1 ./lib/shared/dowork.o -lc 
    rm -f ./lib/shared/libdowork.so.1 ./lib/shared/libdowork.so
    cd ./lib/shared && ln -s ./libdowork.so.1.1 ./libdowork.so.1 && ln -s ./libdowork.so.1.1 ./libdowork.so 
    cd ../../
    g++ -std=c++11 -o ./main.exe ./src/main.cpp -I ./head/ -L./lib/shared/ -ldowork 
    export LD_LIBRARY_PATH=$(pwd)/lib/shared/
    ./main.exe
elif [ "-static" = "$1" ]
then
    g++ -std=c++11 -c -o ./lib/static/dowork.o ./src/dowork.cpp 
    ar -rsc ./lib/static/libdowork.a ./lib/static/dowork.o
    g++ -std=c++11 -c -o ./lib/static/main.o ./src/main.cpp
    g++ ./lib/static/main.o -L./lib/static/ -ldowork -o ./main.static.exe
    ./main.static.exe
fi
