## Platform Driver 
Platform Driver 是 Linux 裡一種針對「不透過標準匯流排（如 PCI 或 USB）」的設備所設計的驅動架構  
常見應用：SoC（如 ARM）的內建硬體：UART、I2C、SPI、GPIO 等控制器、嵌入式系統中固定存在的設備

#### Linux兩種主要驅動類型:
###### 1. Platform Driver（平台驅動）
- 適用於 無法自動偵測的硬體，例如：SoC 上的內建設備（GPIO、UART 等）  
- 需要手動註冊裝置（platform device）與驅動（platform driver）  
- 驅動與裝置的配對，透過 name 或 Device Tree 的 compatible 屬性完成
- 裝置會掛載在 platform bus 上
###### 2. Bus-based Driver（匯流排驅動）
  - 適用於 可自動偵測的硬體，例如：USB 裝置、PCI 裝置、I2C 裝置等
  - 當裝置插入後，系統會自動偵測並通知核心驅動層
  - 驅動與裝置的配對依靠裝置 ID（如 Vendor ID / Device ID )
  - 驅動會掛載在特定的匯流排（bus）下，例如 usb bus、pci bus、i2c bus  

##### platform_device 
  - 描述一個裝置（device）
  - 可以由裝置樹（Device Tree）或 C 程式碼靜態註冊  
##### platform_driver  
  - 描述一個驅動程式  
  - 透過 .probe 方法去初始化設備  
  - .remove 則是設備移除時呼叫  
<br>

## Device Tree  
在嵌入式 Linux 系統中，Device Tree 是用來描述硬體設備資訊的一種結構化資料格式  
功能 : Linux 核心透過 Device Tree 知道有哪些設備存在、它們的參數與如何初始化  
需求 : Linux 核心設計成可通用於不同平台，Device Tree 將硬體資訊與核心程式碼分離，避免將硬體描述寫死在驅動程式中  

#### Device Tree 的檔案與格式  
Device Tree 檔案使用 .dts（Device Tree Source）副檔名，會被編譯成 .dtb（Device Tree Blob）供 Linux 核心讀取  
簡單範例如下:  
<img width="538" height="287" alt="image" src="https://github.com/user-attachments/assets/8e2c943d-be1f-4cf4-9148-97e509dc3037" />  
  - compatible :驅動程式會用這個欄位來比對是否支援該裝置
  - reg :裝置在記憶體中的位址與大小區段
  - status :表示該裝置是否啟用（通常為 "okay" 或 "disabled"）
  - interrupts :	該裝置使用的中斷號
<br>

## 重要指令  
struct platform_driver :整體結構，定義此驅動的資訊與 callback  
struct platform_driver 範例如下:  
<img width="549" height="226" alt="image" src="https://github.com/user-attachments/assets/7ee46b97-d890-4649-bda7-645340e7d9a2" />  
 - .name : 驅動名稱，用來配對非 device tree 的 platform device
 - .of_match_table : 用來和 Device Tree 中的 compatible 比對。 my_of_match 是一個 of_device_id 陣列指標
 - .probe : 設備與驅動配對時呼叫，用來初始化
 - .remove : 設備移除或驅動卸載時呼叫，用來清理資源
<br>

struct of_device_id : 用來描述與 Device Tree 中裝置相條件的結構體
struct of_device_id  範例如下:
<img width="515" height="154" alt="image" src="https://github.com/user-attachments/assets/da7d0c21-b49c-4e78-a025-e8f49c83e136" />  
 - my_of_match[] : 一個裝置表列，列出支援的 compatible 字串  
 - .compatible : 指定這個驅動支援的裝置名稱，會去比對 Device Tree 裡 <compatible> 屬性
 - {} : 結尾的標記，代表這個陣列結束（必要）  
<br>

MODULE_DEVICE_TABLE(type, name) : 在編譯時讓 Linux 核心知道這個模組支援哪些裝置，支援自動載入驅動模組  
 - type ： 裝置類型，像是 of, pci, usb, i2c 等
 - name : struct of_device_id 陣列(定義了這個驅動支援哪些 compatible 字串)
<br>

手動註冊一個 Platform Device  
<img width="602" height="280" alt="image" src="https://github.com/user-attachments/assets/bcc0f578-f776-40da-8291-6a08c28c9f47" />  
struct platform_device_info pdevinfo : 建立的 Platform Device 的資訊  
  - .name : 裝置名稱,和drive.name一樣核心才會幫 device 和 driver 配對起來，並呼叫 driver 的 .probe() 函式
  - .id = -1 : 表示這個裝置不需要 ID
platform_device_register_full(&pdevinfo) : 把 pdevinfo 中的資訊註冊成一個 platform_device
<br>

## 流程
1. Device Tree 定義硬體 ( .dts 或 .dtsi )  
2. Kernel 開機時 parse device tree
3. 建立 Platform Device（由Kernel代為建立）
4. Platform Driver 註冊
5. Kernel 自動做 Device & Driver 的配對
6. probe() 開始驅動初始化
7. 驅動開始使用裝置的資源




