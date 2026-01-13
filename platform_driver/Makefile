obj-m += platform_device.o
obj-m += platform_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 讓 modprobe 抓得到 ko 檔
install:
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/extra/
	sudo cp platform_device.ko platform_driver.ko /lib/modules/$(shell uname -r)/kernel/drivers/extra/
	sudo depmod -a