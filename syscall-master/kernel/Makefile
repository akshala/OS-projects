CONFIG_MODULE_SIG=n

PWD ?= $(shell pwd)
MODULE_NAME=cse231

ifneq ($(KERNELRELEASE),)

$(MODULE_NAME)-y += main.o ksys.o klib.o
obj-m	+= $(MODULE_NAME).o

else

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$(PWD)

endif

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
