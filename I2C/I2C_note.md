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
s32 i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command)  
  -  Linux I2C core 提供的一個 SMBus 協定的封裝函式，用來從指定的 I2C 裝置暫存器中讀取一個 byte  
  -  const struct i2c_client * : 代表要通訊的 I2C 裝置（會含有 I2C bus、裝置地址等資訊） 
  -  u8（0x00~0xFF）: 要讀取的裝置內部暫存器位址（例如溫度暫存器） 
  -  回傳值 0x00 ~ 0xFF:成功，而負數 : 失敗  
<br>

static struct i2c_driver tmp102_sim_driver  
  - 定義一個 I2C 驅動的結構體
  - driver.name : Linux 內部用來比對裝置名稱的欄位，要和後續綁定模擬裝置所用的名稱一致
  - .probe  :  kernel 發現有匹配這個 driver 的裝置時，會呼叫的初始化函式
  - .id_table : 裝置辨識表，用來告訴 kernel 這個 driver 支援哪些裝置名稱，呼叫 .probe 的機制
<br>

module_i2c_driver(tmp102_sim_driver)  
  - 一個macro，是 Linux 核心提供的簡化註冊 I2C driver 的工具
  - 會自動產生如下的程式碼：
<img width="445" height="300" alt="image" src="https://github.com/user-attachments/assets/949b7f76-03ad-472f-b345-c632a1ab166b" />  
<br>

sudo modprobe i2c_stub chip_addr=0x48  
  - kernel會在系統中創建一個虛擬的 I2C adapter (bus)，並且這個 bus 上有一個模擬的 slave device，address 是 0x48
  - modprobe i2c_stub :  Linux kernel中一個 模擬 I2C slave 裝置的模組
  - chip_addr=0x48 : 要模擬的 I2C 裝置地址是 0x48， client driver 在連接到 bus就會找到這個模擬的 slave。

echo tmp102_sim 0x48 | sudo tee /sys/bus/i2c/devices/i2c-0/new_device  
  - 在 i2c-0 bus 上新增一個名稱為 tmp102_sim，地址為 0x48 的 I2C client 裝置
  - tee : 讀取標準輸入並寫入檔案

echo 0-0048 | sudo tee /sys/bus/i2c/devices/i2c-0/delete_device  
  - Kernel 接收到刪除命令，從 i2c-0 bus 中移除名為 1-0048 的裝置
<br>

## 流程
#### Platform Driver 配對流程
  1. Device Tree (.dts) 編寫
  2. Kernel 開機時解析 DTS → 建立 platform_device
  3. platform_driver註冊並配對該 platform_device
  4. .probe() 中呼叫 i2c_add_adapter() 註冊 I2C bus → 建立 i2c_adapter
  5. i2c-core 掃描 DTS，找出與此 adapter 相關的 i2c_client
  6. 建立對應的 i2c_client → 掛載在某個 i2c_adapter（i2c bus）上
  7. 註冊的 i2c_driver 透過 of_match_table 或 id_table 比對
  8. 在 i2c_driver 的 .probe() 中，開始與硬體溝通



















 
