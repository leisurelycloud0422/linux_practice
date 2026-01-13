# 模組名稱，必須與你的 .c 檔名一致
obj-m += fake_hwmon.o

# 核心開發路徑與目前路徑
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 安裝模組到系統目錄，方便使用 modprobe 測試
install:
	sudo cp fake_hwmon.ko /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/
	sudo depmod -a

# 移除系統目錄中的模組
uninstall:
	sudo rm -f /lib/modules/$(shell uname -r)/kernel/drivers/hwmon/fake_hwmon.ko
	sudo depmod -a