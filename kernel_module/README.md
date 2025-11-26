## 📑 目錄
- **Linux Kernel Module**
- **Makefile**
- **重要指令**
- **真實嵌入式驅動方式**  
<br><br>


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
###### module_init()：模組被載入時執行的初始化函數  
###### module_exit()：模組被卸除時執行的清理函數  
###### insmod :把.ko 模組載入 Linux 核心
###### modprobe :智慧載入模組，會自動處理依賴
###### rmmod :移除模組  
###### dmesg :查看 kernel log（動態日誌） 
###### modinfo :查看模組的靜態資訊 

##### insmod vs. modprobe
| 動作 | insmod | modprobe |
| --- | --- | --- |
| 載入模組方式 | 直接載入 `.ko` 檔 | 根據「模組名稱」載入 |
| 路徑 | **需要指定 `.ko` 的完整路徑** | 自動在 `/lib/modules/$(uname -r)/` 底下搜尋 |
| 相依性 (dependencies) | ❌ 不會處理 | ✔ 自動載入相依模組 |
| 移除模組 | `rmmod` | `modprobe -r`，也會卸載相依鏈 |
| 設定 / blacklisting | 不支援 | ✔ 支援 `/etc/modprobe.d/*.conf` |
| 使用場景 | Driver 開發測試 | 正式系統、BMC、嵌入式系統啟動流程 |
<br><br>

###### MODULE_LICENSE("GPL") :告訴 kernel 是開源模組  
###### MODULE_AUTHOR(...) :註明作者  
###### MODULE_DESCRIPTION(...) :模組簡介  
###### MODULE_PARM_DESC(x,"...") :描述module parameter的說明文字的巨集,這個說明會在使用者查詢模組資訊時顯示出來，例如用 modinfo 指令查看 .ko 模組時  
<br><br>
###### module_param(name, type, perm):                  // 加入參數
  - name: 變數名稱
  - type: 支援 int, charp, bool, long, short, uint, ulong, ushort 等
  - perm: 權限（檔案 /sys/module/<modname>/parameters/ 的屬性），如：
    * 000: 不允許從 /sys/ 修改
    * 0644: 使用者可讀寫
<br><br>

## 真實嵌入式驅動方式  
###### 1️⃣ 確認硬體與資料手冊  
- 先搞清楚晶片/外設接在哪裡 (I2C/SPI/UART/GPIO/PCIe…)
- 查 datasheet / reference manual，知道
    - 寄存器地址 (register map)
    - 初始化順序
    - 通訊協議需求
###### 2️⃣ Device Tree 定義硬體

- 這樣 kernel 啟動時才會「認識」板子上有這顆裝置
- 查看設備樹節點 在開發版上輸入 ls /proc/device-tree/  
👉 **真實開發差別**：模擬環境可以直接呼叫驅動，但實機一定要靠 **DT 描述硬體**，驅動才會被綁定
###### 3️⃣ 撰寫驅動程式 (Driver Code)
- 新增一個 `.c` 驅動檔：
    - 使用 `platform_driver_register()` → 比對 `compatible`
    - 在 `probe()` 裡做硬體初始化
    - 提供 `read()/write()/ioctl()` 等 file_operations  
##### 4️⃣ 編譯 & 部署驅動
- 使用交叉編譯器 `arm-linux-gnueabihf-gcc` 把 `.c` 編成 `.ko`
- 複製到板子（NFS/SD 卡/SSH）
- `insmod mydev.ko` → 驅動載入成功
👉 **模擬環境 (x86)** 可以直接 `make`，
👉 **真實嵌入式** 必須用 **交叉編譯器**，因為目標 CPU 不是 x86
##### 5️⃣ 驗證 /sys 與 /dev 節點
- 驅動註冊成功後會出現：
    - `/sys/class/mydev/...` → 驅動屬性檔 (用來 debug / 調參數)
    - `/dev/mydev` → 真正提供給使用者空間 App 存取的裝置檔
    👉 真實情況下，這些節點是由驅動 + udev 自動建立
##### 6️⃣ 撰寫應用程式 (User-space App) 
- 透過 /dev/mydev 進行操作：
👉 **為何要 App？**
因為 kernel driver 只是提供介面，實際的業務邏輯 / 顯示 / 傳輸，要靠 **User-space App**
##### 7️⃣ 測試 & Debug
- 驗證功能正確性
- 用 `dmesg` 看驅動 log (`pr_info`, `pr_err`)
- 用 `/sys` 節點即時調整參數（不用重編）
- 遇到 bug → 加 trace、用 `ftrace`、`gdb`、`strace` 協助分析












