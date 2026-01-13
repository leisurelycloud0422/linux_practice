# 假設你的原始檔名是 fan_ctrl.c
obj-m += fan_ctrl.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
# 修正部署路徑
install:
	sudo mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/extra/
	sudo cp fan_ctrl.ko /lib/modules/$(shell uname -r)/kernel/drivers/extra/
	sudo depmod -a