# 這裡必須對應你的檔名 new_vir0511h.c
obj-m += new_vir0511h.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/
	sudo cp new_vir0511h.ko /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/
	sudo depmod -a