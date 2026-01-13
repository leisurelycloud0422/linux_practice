# 假設你的原始檔名是 modern_way.c，如果不是，請修改這裡的名稱
obj-m += newway.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 自動測試工具
test:
	sudo dmesg -C
	sudo insmod modern_way.ko
	sudo chmod 666 /dev/mydev
	ls -l /dev/mydev
	sudo dmesg | grep "mydev"