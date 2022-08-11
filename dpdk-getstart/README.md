# DPDK get start

# Supported NICs

reference `https://core.dpdk.org/supported/`

# Support on virtual machine

1.change the default virtual nic from e1000 to vmxnet3

**e1000 only support one tx、rx queue, vmxnet3 support 4 tx、rx queue**

shutdown vm, then edit the vm .vmx file that is vm configuration file(in the vm installation folder), change **ethernetX.virtualDev = **, from **e1000** to **vmxnet3**

2.display nic numbers of tx、rx queues

`cat /proc/interrupts | grep ens`

3.create virtual network for thoese nics

you can create any host only network for thoese nics for communication.

Select vmware menu -> select **Edit** -> virtual network editor

# Download and compile

reference 'https://core.dpdk.org/doc/quick-start/'.

Notes:
1. the make config stage will not generate $RTE_TARGET directory, just generate './build' directory that it contains all of compiling files such as .o, include file, libraries. 
so we need to create it($RTE_TARGET, in common condition, it is x86_64-native-linuxapp-gcc) manually. 
and after us finished compiling. we need to copy ./build/.config ./build/.config.orig ./build/include/ ./build/lib/ to $RTE_TARGET

2. if we configure `CONFIG_RTE_LIBRTE_PMD_PCAP=y` in ./build/.config after compiling.
like this below:
```
make config T=x86_64-native-linuxapp-gcc
sed -ri 's,(PMD_PCAP=).*,\1y,' build/.config
```
My all of applications what are using dpdk library(dpdk applications) in the future will dependency libpcap library.

# How to run

## 1. insert dpdk kernel module

```
modprobe igb_uio
insmod $(DPDK_PROJECT)/build/kmod/igb_uio
```

## 2. bind dpdk driver to NIC

```
#suppose the nic name is eth1, it's pci address is 0000:01:00.0 (domain:bus:slot.func). we can use 'lspci -v|grep Ether' command find pci address out.
$(DPDK_PROJECT)/usertools/dpdk-devbind.py --bind=igb_uio eth1
#then you can see the driver has been binded to eth1, like the output below
ll /sys/bus/pci/devices/0000\:01\:00.0/driver
lrwxrwxrwx  1 root  root  0 May 12  15:51 /sys/bus/pci/devices/0000:01:00.0/driver -> ../../../../bus/pci/drivers/igb_uio
```

alternate binding method.
reference 'https://lwn.net/Articles/143397/'

```
#unbind it firstly.
echo -n '0000:01:00.0' > /sys/bus/pci/drivers/e1000/unbind
#bind it
echo -n '0000:01:00.0' > /sys/bus/pci/drivers/igb_uio/bind
```

## 3. start dpdk application

many example applications and it's usages can be found in `https://doc.dpdk.org/guides/sample_app_ug/`.
if you are a newbie in dpdk, you can follow these examples to get start.

```
#suppose the application name is test
# -l: which cores are dpdk running on
# -n: how many NUMA(None uniform memory access) nodes can be used. "lscpu | grep 'NUMA node'"
test -l0,1 -n1

# the application will automatically use these nics what are using 'igb_uio' driver after us binded it above.
# in our example. just one port(Netcard port) be used. one nic one port.
```

## 4. dpdk need hugepages to run

refer to `experiences/hugepage` to set up hugepages

# How to compile in c++

These below flags must be added in Makefile, otherwise will arise error `error: expected ')' before 'PRIu64'`

```
CFLAGS += -D__STDC_LIMIT_MACROS
CFLAGS += -D__STDC_FORMAT_MACROS
```

# Issues

## 1. in vmware. `EAL: Error enabling interrupts for fd 10 (Input/output error)`

reference `https://dev.to/dannypsnl/dpdk-eal-input-output-error-1kn4`

```
DPDK – EAL Input/output error
Last week I’m trying to reproduce a bug happened in our customer environment, so we create a minimal example for this: https://github.com/glasnostic/nff\_go\_test

During this, I found an annoying problem and want to record it.

I got an error: EAL: Error enabling interrupts for fd 10 (Input/output error)

After some research, I found a patch for this(it didn’t be merged into DPDK since it’s a VMWare problem).

If you try to bind NIC that using e1000 you might have the same issue.

To solve this disables the checking by:
sed -i "s/pci_intx_mask_supported(dev)/pci_intx_mask_supported(dev)||1/g" \
  $(DPDK_PROJECT)/kernel/linux/igb_uio/igb_uio.c

This would make pci_intx_mask_supported check do not work anymore.

then recompile, after compiling done, reload the kernel module:
rmmod igb_uio
insmod $(DPDK_PROJECT)/build/kmod/igb_uio

p.s. DPDK_PROJECT is the project root directory of DPDK, related to your environment.

Then this problem should be fixed.
```
