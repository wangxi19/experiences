# Usage of hugepage in linux

When a process uses some memory, the CPU is marking the RAM as used by that process. For efficiency, the CPU allocate RAM by chunks of 4K bytes (it's the default value on many platforms). Those chunks are named pages. Those pages can be swapped to disk, etc.

Since the process address space are virtual, the CPU and the operating system have to remember which page belong to which process, and where it is stored. Obviously, the more pages you have, the more time it takes to find where the memory is mapped. When a process uses 1GB of memory, that's 262144 entries to look up (1GB / 4K). If one Page Table Entry consume 8bytes, that's 2MB (262144 * 8) to look-up.

Most current CPU architectures support bigger pages (so the CPU/OS have less entries to look-up), those are named Huge pages (on Linux), Super Pages (on BSD) or Large Pages (on Windows), but it all the same thing.


## How to get and use hugepages

```shell
##1. display hugepage information
grep "Huge" /proc/meminfo
#ShmemHugePages:        0 kB
#HugePages_Total:     0
#HugePages_Free:      0
#HugePages_Rsvd:        0
#HugePages_Surp:        0
#Hugepagesize:       2048 kB
#Hugetlb:          0 kB

##2. make hugepages
##how many hugepages do you want to get?
##256 number hugepages, and each hugepage size is 2MB(the default size)
cat >> /etc/sysctl.conf << EOF
vm.nr_hugepages=256
EOF

##then execute sysctl -p to reload this configuration. if you have good luck (the sequential memory is large enough), you will get the hugepages that you want. but in normal condition, you willn't get the eough hugepages, you just get a little. because the sequential memory isn't large enough. so when you display the holding hugepages' number you will see the numbers is less than 256.

sysctl -p; grep "HugePages_Total" /proc/meminfo
#HugePages_Total:     29

##or you can execute the cmd to get the same result
sysctl -p; cat /proc/sys/vm/nr_hugepages
#29

## if you want to get full of hugepages, reboot is the most reliable method.

reboot

cat /proc/sys/vm/nr_hugepages
#256

## but if you don't want reboot, try to execute this below repeatedly until you get enough hugepages.(pure luck, if you stop more programs, the higher your probability)
sync; echo 3 > /proc/sys/vm/drop_caches; sysctl -p; cat /proc/sys/vm/nr_hugepages
#56
sync; echo 3 > /proc/sys/vm/drop_caches; sysctl -p; cat /proc/sys/vm/nr_hugepages
#106
## ...

##3. mount hugepages.
##in most recent operating systems (such as debian 9), hugetlbfs has been mounted to /dev/hugepages. in the condition, skip this step.
#mountpoint /dev/hugepages; if [ `echo $?` != 0 ]; then mount -t hugetlbfs -o uid=<value>,gid=<value>,mode=<value>,pagesize=<value>,size=<value>,min_size=<value>,nr_inodes=<value> none /dev/hugepages; else echo "hugetlbfs has mounted to /dev/hugepages"; fi;
mkdir -p /dev/hugepages
mountpoint /dev/hugepages; if [ `echo $?` != 0 ]; then mount -t hugetlbfs -o mode=1770 none /dev/hugepages; else echo "hugetlbfs has mounted to /dev/hugepages"; fi;

##4. use hugepages
##use 10 hugepages
fallocate --version
#fallocate from util-linux 2.34
fallocate -l $((2*1024*1024*10)) /dev/hugepages/hg1
#grep "HugePages_Total - HugePages_Free" == 10
##Note. if you use truncate -s $((2*1024*1024*10)) /dev/hugepages/hg1.
#grep "HugePages_Total - HugePages_Free" == 0. because the file punch hole

## when you preallocate fixed size hg file, you can use it through mmap (in c++, see the main.cpp), using the hg file is equal to using hugepage memory directly
## or you also haven't to preallocate fixed size hg file, you can use open(c++) to create a empty hg file, and then use mmap's length parameter to enlarge the hg size.
##if use MAP_SHARED FLAG.
##Share  this  mapping.   Updates to the mapping are visible to other processes mapping
##the same region, ***and (in the case of file-backed mappings) are carried through to the underlying  file.***
## The role of mmap is to map the physical address to the memory. operation on the memory address is synchronous to the mapped physical address. in this condition the 'physical address' is the hugepage memory. so you operate the mapped address is equal to operate hugepages memory
```

## How to free used hugepages

some times, hugepages are still used after my application exited.
`cat /proc/meminfo | grep Huge`

```
AnonHugePages:    759808 kB
ShmemHugePages:        0 kB
HugePages_Total:    1024
HugePages_Free:     1023  //one hugepage is used
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:         2097152 kB
```
but all applications which use hugepages exited. Why is the hugepage still used?
the reason is that the hugepage file still eixts. if delete the file, hugepage will be freed
`mount -l |grep hugetlbfs`
```
hugetlbfs on /dev/hugepages type hugetlbfs (rw,relatime,pagesize=2M)
```

`ls -lh /dev/hugepages`
```
total 2.0M
-rw------- 1 root root 2.0M May  9 03:19 rtemap_0
```

`rm -f /dev/hugepages/* && cat /proc/meminfo | grep Huge`
```
AnonHugePages:    761856 kB
ShmemHugePages:        0 kB
HugePages_Total:    1024
HugePages_Free:     1024  //that hugepage was freed already
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:         2097152 kB
```


## External link

https://elixir.bootlin.com/linux/latest/source/tools/testing/selftests/vm/map_hugetlb.c
https://elixir.bootlin.com/linux/latest/source/tools/testing/selftests/vm/hugepage-shm.c
https://elixir.bootlin.com/linux/latest/source/tools/testing/selftests/vm/hugepage-mmap.c
