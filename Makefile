# 2014122011 Eunsoo Sheen
obj-m += hw2.o
hw2-objs := ./src/hw2.o ./src/buddyinfo.o ./src/virtmeminfo.o 
hw2-objs += ./src/rssarray.o
# ccflags-y += -DCONFIG_SPARSE_IRQ=y 
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
install:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
