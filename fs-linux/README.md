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
for $((i=0;i<100;i++)); do fallocate -l $((4096*1024*100)) ./${i}.fstor; done;
```
