PHONY:
	show all

all:
	gcc -g3 -O0 -c -o foo.o ./foo.c
	gcc -g3 -O0 -fPIC -c -o foo.fpic.o ./foo.c
	gcc -g3 -O0 -c -o oof.o ./oof.c
	gcc -g3 -shared -o libfoof.so ./foo.o ./oof.o || (echo "Must use fpic to compose shared lib" && gcc -shared -o libfoof.so ./foo.fpic.o ./oof.o)
	gcc -g3 -O0 -o main ./main.c -L./ -l:libfoof.so -Wl,-rpath=./

show: all
	readelf -r foo.o
	readelf -r foo.fpic.o
	readelf -r oof.o
	readelf -r libfoof.so
	readelf -r main
