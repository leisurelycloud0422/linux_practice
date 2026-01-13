# 核心模組名稱
obj-m += tmp102_sim.o

# 核心原始碼路徑
KDIR := /lib/modules/$(shell uname -r)/build
# 目前目錄
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 使用 modprobe 前的安裝步驟
install:
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/extra/
	sudo cp tmp102_sim.ko /lib/modules/$(shell uname -r)/kernel/drivers/extra/
	sudo depmod -a

# 移除安裝
uninstall:
	sudo rm /lib/modules/$(shell uname -r)/kernel/drivers/extra/tmp102_sim.ko
	sudo depmod -a