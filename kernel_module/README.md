## Linux kernel module 

執行在 核心模式（Kernel Mode） 的程式模組，有別於一般在 使用者模式（User Mode） 執行的程式（process）。

##### 主要用途：
用來擴充核心功能，多數是硬體驅動程式（Driver），例如裝置的控制程式。

##### 設計理念：
可依需求載入或移除，不需重編核心本體（modular design）
<br><br>

## Makefile
是一種自動化編譯的腳本檔，不直接編譯.c 檔，而是扮演一個「橋梁」的角色，主要作用是告訴 Linux kernel 的 build system：「我要編譯這個 module」。  
make :讀取 Makefile 並執行其中的編譯指令  
###### /lib/modules/$(uname -r)/build 
這個路徑是 Linux 安裝的 kernel headers 的位置。裡面有：  
  - Makefile：讓你能用 kernel 的規則來編譯模組  
  - include/：核心的標頭檔（像 linux/module.h 就在這裡）  
  - 編譯.c 成.ko 所需要的一切設定  

## 重要指令
module_init()：模組被載入時執行的初始化函數  
module_exit()：模組被卸除時執行的清理函數  
MODULE_LICENSE("GPL") :告訴 kernel 是開源模組  
MODULE_AUTHOR(...) :註明作者  
MODULE_DESCRIPTION(...) :模組簡介  
MODULE_PARM_DESC(x,"...") :描述module parameter的說明文字的巨集,這個說明會在使用者查詢模組資訊時顯示出來，例如用 modinfo 指令查看 .ko 模組時  
insmod :載入模組  
rmmod :移除模組  
dmesg :查看 kernel log（動態日誌） 
modinfo :查看模組的靜態資訊 
<br><br>
module_param(name, type, perm):                  // 加入參數
  - name: 變數名稱
  - type: 支援 int, charp, bool, long, short, uint, ulong, ushort 等
  - perm: 權限（檔案 /sys/module/<modname>/parameters/ 的屬性），如：
    * 000: 不允許從 /sys/ 修改
    * 0644: 使用者可讀寫
##

