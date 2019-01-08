name = test.o
#name ?= jprobe.o
obj-m := $(name)
KERNELDIR := /data/lyq/s1s/kernel-4.4/
ARCH = powerpc
export ARCH CROSS_COMPILE
#export CC
default:
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	make -C $(KERNELDIR) M=$(shell pwd) clean
