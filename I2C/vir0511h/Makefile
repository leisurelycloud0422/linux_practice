# 指定模組名稱
obj-m += vir0511h.o

# 核心路徑與當前路徑
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 安裝模組
install:
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/
	sudo cp vir0511h.ko /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/
	sudo depmod -a

# 移除模組
uninstall:
	sudo rm /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/vir0511h.ko
	sudo depmod -a