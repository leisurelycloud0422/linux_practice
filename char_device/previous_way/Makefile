# 模組名稱（對應你的原始檔名 oldway.c）
obj-m += oldway.o

# 取得目前運行的核心版本
KDIR := /lib/modules/$(shell uname -r)/build
# 取得目前的檔案路徑
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 測試用：方便你快速查看訊息
log:
	sudo dmesg -C
	sudo insmod oldway.ko
	sudo dmesg
	lsmod | grep oldway