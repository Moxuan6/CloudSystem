arch ?= arm
modname ?= ap3216
cc := arm-none-linux-gnueabihf-gcc

ifeq ($(arch),arm)
	KERNELDIR:=/home/jeremy/linux-5.10.61  #开发板
else
	KERNELDIR:=/lib/modules/$(shell uname -r)/build/  #Ubuntu
endif
PWD:= $(shell pwd)

all:
	make -C $(KERNELDIR) M=$(PWD) modules
	@rm -rf .*.ko.cmd .*.mod.cmd .*.mod.o.cmd .*.o.cmd .Module.symvers.cmd .modules.order.cmd *.mod *.mod.c *.mod.o *.o Module.symvers modules.order
	$(cc) -o $(modname)-test test.c
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -rf $(modname)-test
install:
	cp $(modname).ko /home/jeremy/rootfs
	cp $(modname)-test /home/jeremy/rootfs

obj-m:=$(modname).o #当前目录下的模块名