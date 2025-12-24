## I2C
 I2C 架構 4 元件：  
 ##### 1. adapter  	
       定義：表示一個實際的 I2C 控制器（例如 SoC 上的某個 I2C 硬體）。
       由誰註冊：Platform driver 或 I2C bus driver。
       代表：一條 I2C Bus（實體匯流排），每個 adapter 對應一條 bus。
       結構體：struct i2c_adapter
       重要欄位：
       - name：名稱
       - algo：指向 i2c_algorithm 的指標，用於低階操作
       - nr：bus 編號（如 /dev/i2c-0）  
 ##### 2. algorithm : 控制 I2C 傳輸的方法
       定義：adapter 所使用的底層傳輸函式介面。
       角色：提供實作函式給核心，例如 master_xfer() 來收發資料。
       結構體：struct i2c_algorithm
       重要函式：
       - master_xfer()：傳輸函式
       - functionality()：告訴核心此 controller 支援哪些功能
 ##### 3. client : 代表一個 I2C 裝置，例如 TMP102 溫度感測器  
       定義：描述一個 I2C 裝置（client/slave device）
       由誰建立：可以由 Device Tree、自動探測、或 i2c_new_client_device()
       結構體：struct i2c_client
       重要欄位：
       - addr：裝置位址（如 0x48）
       - adapter：對應的 i2c_adapter 指標
       - driver：指向綁定的 i2c_driver
 ##### 4. driver : 操作特定裝置（client）的邏輯驅動程式  
       定義：負責與某類 client 裝置溝通的程式邏輯。
       由誰註冊：驅動程式模組（driver module）會呼叫 i2c_add_driver() 註冊。
       結構體：struct i2c_driver
       重要欄位：
       - probe()：client 裝置被偵測或註冊時呼叫
       - id_table：支援的裝置清單（如 {"tmp102", 0}）
<br>

## 模擬 vs 真實 Device Tree 差異整理  
#### 模擬 Device  
 - 用途：方便測試 Driver 不需要實體硬體
 - 方式：手動建立 I2C client device（寫入 /sys/bus/i2c/devices/i2c-*/new_device） 
 - 立即見效：系統會立刻呼叫相符的 i2c_driver 的 probe() 函式  
 - 非永久：重開機或重載 i2c bus driver 就會消失  
 - 不需 Device Tree 支援  
#### 真實 Device Tree 
 -  用途：描述實際硬體資訊，讓 kernel 知道要啟用哪些設備  
 -  方式：在 .dts 檔中撰寫裝置資訊，並在 build 時合併進 kernel 或 device tree blob (.dtb)  
 -  啟動自動掛載：開機階段 kernel 解析 device tree，自動與驅動配對（of_match_table）  
 -  適用大量裝置：像 PMIC、I2C sensor、LED、Fan、GPIO 等實體元件  
 -  需重編 kernel 或 dtb 檔案：修改麻煩，但正式產品必備  
<br>

## 重要指令
#### ✅`int i2c_master_send(struct i2c_client *client, const char *buf, int count); `   
向 I2C 裝置 (slave) 發送資料
相當於在 bus 上傳送 count 個 byte
 - client : 指向 i2c_client 的指標，表示要操作的裝置
 - buf : 要發送的資料緩衝區
 - count : 要發送的 byte 數量
 - 成功 → 發送的 byte 數量
   失敗 → 負數錯誤碼，例如 -EIO
<br>

#### ✅`int i2c_master_recv(struct i2c_client *client, char *buf, int count); `  
從 I2C 裝置 (slave) 接收資料
會自動加上 start condition 和 stop condition
一般用在 讀取 ADC / sensor 資料 的情境
 - client : 指向 i2c_client 的指標
 - buf : 資料接收緩衝區，會把裝置傳回的資料放在這裡
 - count : 要讀取的 byte 數量
 - 成功 → 實際接收的 byte 數量
 - 失敗 → 負數錯誤碼
<br>

#### ✅` static int i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)`  
I2C 裝置被 kernel 掃描到後呼叫
probe() 是 driver 與 I2C 裝置連接的起點。
分配 driver 私有資料、存 client 指標、初始化 mutex，然後註冊 hwmon 裝置，這樣使用者空間就可以透過 sysfs 讀取感測器值   
<br>

#### ✅` static int i2c_remove(struct i2c_client *client)`  
I2C 裝置被移除或 module 卸載時呼叫
清理 driver 使用的資源
 - devm_hwmon_device_register_with_info 自動解除註冊
 - devm_kzalloc 自動釋放記憶體  
<br>

#### ✅` static struct i2c_driver vir0511h_driver`
```
static struct i2c_driver vir0511h_driver = {
    .driver = {
        .name = "vir0511h",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(vir0511h_of_match),
    },
    .probe = vir0511h_probe,
    .remove = vir0511h_remove,
    .id_table = vir0511h_id,
}; 
```
 - .driver : 描述這個 driver 的基本資訊，讓 kernel 可以管理它  
  - .name : driver 名稱，kernel log 或 sysfs 會用到  
  - .owner : module 所屬，使用 THIS_MODULE 可讓 module 不被卸載時被 kernel 認識
  - .of_match_table : device tree 匹配表，如果使用 DT 描述裝置，就用這個來匹配裝置
 - .probe : 裝置匹配 driver 後呼叫 , 初始化 driver 私有資料、mutex、hwmon 註冊等
 - .remove : 裝置被移除或 module 卸載時呼叫
 - .id_table : 傳統 I2C 裝置 ID 匹配表 , 如果沒有 device tree，kernel 就透過這個 table 找到 driver
<br>





<br>

## 流程
#### Platform Driver 配對流程
  1. Device Tree (.dts) 編寫
  2. Kernel 開機時解析 DTS → 建立 platform_device
  3. platform_driver註冊並配對該 platform_device
  4. 硬體控制器 driver 初始化 ,.probe() 中會初始化硬體寄存器，建立 i2c_adapter → 代表某個 I2C bus。
  5. .probe() 中呼叫 i2c_add_adapter() 註冊 I2C bus → 建立 i2c_adapter
  6. i2c-core 掃描 DTS，找出與此 adapter 相關的 i2c_client
  7. 建立對應的 i2c_client → 掛載在某個 i2c_adapter（i2c bus）上
  8. 註冊的 i2c_driver 透過 of_match_table 或 id_table 比對
  9. 在 i2c_driver 的 .probe() 中，開始與硬體溝通



















 
