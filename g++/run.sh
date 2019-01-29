#!/bin/bash
test -d ./lib
if [ 1 -eq $? ]
then
    mkdir ./lib && mkdir ./lib/shared && mkdir ./lib/static
fi

if [ "-shared" = "$1" ]
then
    g++ -pthread -std=c++11 -fPIC -c -o ./lib/shared/dowork.o ./src/dowork.cpp -lc -latomic -O2
    g++ -pthread -shared -fPIC -Wl,-soname,libdowork.so.1 -o ./lib/shared/libdowork.so.1.1 ./lib/shared/dowork.o -lc -latomic
    rm -f ./lib/shared/libdowork.so.1 ./lib/shared/libdowork.so
    cd ./lib/shared && ln -s ./libdowork.so.1.1 ./libdowork.so.1 && ln -s ./libdowork.so.1.1 ./libdowork.so 
    cd ../../
    g++ -pthread -std=c++11 -o ./main.exe ./src/main.cpp -I ./head/ -lc -latomic -L./lib/shared/ -ldowork 
    export LD_LIBRARY_PATH=$(pwd)/lib/shared/
    ./main.exe
elif [ "-static" = "$1" ]
then
    g++ -pthread -std=c++11 -c -o ./lib/static/dowork.o ./src/dowork.cpp -lc -latomic -O2
    ar -rsc ./lib/static/libdowork.a ./lib/static/dowork.o
    g++ -pthread -std=c++11 -c -o ./lib/static/main.o ./src/main.cpp -lc -latomic
    g++ -pthread ./lib/static/main.o -lc -latomic -L./lib/static/ -ldowork -o ./main.static.exe 
    ./main.static.exe
elif [ "" = "$1" ]
then
    echo "run.sh [-shared | -static]"
fi
