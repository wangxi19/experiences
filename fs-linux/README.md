# How to preallocate fixed size and fixed quantity files which are in successive physical address of disk

Use usb device to illustrate;

## Step1: find out usb device from /dev/ folder

```shell
lsblk
#NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
#sda      8:0    0  100G  0 disk 
#├─sda1   8:1    0 98.1G  0 part /
#├─sda2   8:2    0    1K  0 part 
#└─sda5   8:5    0  1.9G  0 part [SWAP]
#sdb      8:16   1 14.4G  0 disk /tmp/m       <-- this is my usb device
#sr0     11:0    1 1024M  0 rom 
```

## Step2: make filesystem for usb device
```shell
mkfs -t ext4 /dev/sdb
#mke2fs 1.45.3 (14-Jul-2019)
#/dev/sdb contains a ext4 file system
#	last mounted on /tmp/m on Wed Oct  9 03:57:27 2019
#Proceed anyway? (y,N) y
#/dev/sdb is mounted; will not make a filesystem here!    <--- we need to umount the usb device before make a new filesystem for it
```
if usb device has been mounted, umount it
```shell
#use `df` or `lsblk` to find the mount position
df -h
#Filesystem      Size  Used Avail Use% Mounted on
#udev            1.9G     0  1.9G   0% /dev
#tmpfs           392M   12M  381M   3% /run
#/dev/sda1        97G   12G   80G  13% /
#tmpfs           2.0G  132M  1.8G   7% /dev/shm
#tmpfs           5.0M     0  5.0M   0% /run/lock
#tmpfs           2.0G     0  2.0G   0% /sys/fs/cgroup
#tmpfs           392M   16K  392M   1% /run/user/132
#tmpfs           392M   48K  392M   1% /run/user/0
#/dev/sdb         15G  431M   13G   4% /tmp/m       <---`/tmp/m/` this is the mount position of usb device

#or use lsblk
#lsblk
#NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
#sda      8:0    0  100G  0 disk 
#├─sda1   8:1    0 98.1G  0 part /
#├─sda2   8:2    0    1K  0 part 
#└─sda5   8:5    0  1.9G  0 part [SWAP]
#sdb      8:16   1 14.4G  0 disk /tmp/m             <---`/tmp/m/` this is the mount position of usb device  
#sr0     11:0    1 1024M  0 rom  

umount /tmp/m && mkfs -t ext4 /dev/sdb
```

then we has been made a ext4 file system for usb device

## Step3: mount usb device
```shell
mount /dev/sdb /tmp/m
```

## Step4: use fallocate to generate fixed size file. Note: file size should align file system block size
```shell
#Get file system block size firstly
blockdev --getbsz /dev/sdb
#4096

#Then allocate fixed size file, the file hasn't punch hole in my condition
cd /tmp/m
#example: we preallocate 100 files which has fixed size and are in successive physical address of usb disk
for ((i=0;i<100;i++)); do fallocate -l $((4096*1024*100)) ./${i}.fstor; done;
```

## Step5: Check these files whether are in successive physical address
```shell
filefrag -v /tmp/m/0.fstor /tmp/m/1.fstor /tmp/m/2.fstor
#Filesystem type is: ef53
#File size of /tmp/m/0.fstor is 4096000 (1000 blocks of 4096 bytes)
# ext:     logical_offset:        physical_offset: length:   expected: flags:
#   0:        0..     999:      99331..    100330:   1000:             last,unwritten,eof
#/tmp/m/0.fstor: 1 extent found
#File size of /tmp/m/1.fstor is 4096000 (1000 blocks of 4096 bytes)
# ext:     logical_offset:        physical_offset: length:   expected: flags:
#   0:        0..     999:     100331..    101330:   1000:             last,unwritten,eof
#/tmp/m/1.fstor: 1 extent found
#File size of /tmp/m/2.fstor is 4096000 (1000 blocks of 4096 bytes)
# ext:     logical_offset:        physical_offset: length:   expected: flags:
#   0:        0..     999:     101331..    102330:   1000:             last,unwritten,eof
#/tmp/m/2.fstor: 1 extent found

##we can see that these's physical address are successive: 99331..    100330 100331..    101330 101331..    102330

##we can use `ls` or `du` to display the number of blocks that has been used with each file
##we will see that 1000 blocks are used with each file. (because my file fixed size is 4096*1000 (4MB))
ls -s --block-size=4096 /tmp/m/
total 100000
#1000 0.fstor   1000 18.fstor  1000 26.fstor  1000 34.fstor  1000 42.fstor  1000 50.fstor  1000 59.fstor  1000 67.fstor  #1000 75.fstor  1000 83.fstor  1000 91.fstor  1000 9.fstor
#1000 10.fstor  1000 19.fstor  1000 27.fstor  1000 35.fstor  1000 43.fstor  1000 51.fstor  1000 5.fstor   1000 68.fstor  #1000 76.fstor  1000 84.fstor  1000 92.fstor
#1000 11.fstor  1000 1.fstor   1000 28.fstor  1000 36.fstor  1000 44.fstor  1000 52.fstor  1000 60.fstor  1000 69.fstor  #1000 77.fstor  1000 85.fstor  1000 93.fstor
#1000 12.fstor  1000 20.fstor  1000 29.fstor  1000 37.fstor  1000 45.fstor  1000 53.fstor  1000 61.fstor  1000 6.fstor   #1000 78.fstor  1000 86.fstor  1000 94.fstor
#1000 13.fstor  1000 21.fstor  1000 2.fstor   1000 38.fstor  1000 46.fstor  1000 54.fstor  1000 62.fstor  1000 70.fstor  #1000 79.fstor  1000 87.fstor  1000 95.fstor
#1000 14.fstor  1000 22.fstor  1000 30.fstor  1000 39.fstor  1000 47.fstor  1000 55.fstor  1000 63.fstor  1000 71.fstor  #1000 7.fstor   1000 88.fstor  1000 96.fstor
#1000 15.fstor  1000 23.fstor  1000 31.fstor  1000 3.fstor   1000 48.fstor  1000 56.fstor  1000 64.fstor  1000 72.fstor  #1000 80.fstor  1000 89.fstor  1000 97.fstor
#1000 16.fstor  1000 24.fstor  1000 32.fstor  1000 40.fstor  1000 49.fstor  1000 57.fstor  1000 65.fstor  1000 73.fstor  #1000 81.fstor  1000 8.fstor   1000 98.fstor
#1000 17.fstor  1000 25.fstor  1000 33.fstor  1000 41.fstor  1000 4.fstor   1000 58.fstor  1000 66.fstor  1000 74.fstor  #1000 82.fstor  1000 90.fstor  1000 99.fstor

#or use du -s --block-size=4096 /tmp/m/* 
du -s --block-size=4096 /tmp/m/* 
#1000	/tmp/m/0.fstor
#1000	/tmp/m/10.fstor
#1000	/tmp/m/11.fstor
#1000	/tmp/m/12.fstor

#Note: these block quantities in du or ls are equivalent because these files haven't punch hole
```
