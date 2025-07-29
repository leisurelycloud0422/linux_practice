## I2C
 I2C 架構 4 元件：  
 1.adapter : 	代表主機端的 I2C 控制器  
 2.algorithm : 控制 I2C 傳輸的方法  
 3.client : 代表一個 I2C 裝置，例如 TMP102 溫度感測器  
 4.driver : 操作特定裝置（client）的邏輯驅動程式  
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
