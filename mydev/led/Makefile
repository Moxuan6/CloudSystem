arch ?= arm
modname ?= led
ifeq ($(arch),arm)
	KERNELDIR:=/home/jeremy/linux-5.10.61  #开发板
else
	KERNELDIR:=/lib/modules/$(shell uname -r)/build/  #Ubuntu
endif
PWD:= $(shell pwd)

all:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
install:
	cp $(modname).ko /home/jeremy/rootfs
obj-m:=$(modname).o #当前目录下的模块名