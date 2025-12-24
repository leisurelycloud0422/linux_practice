## hwmon 子系統
- **用途**：提供統一介面讀取硬體監控資訊，例如溫度、風扇轉速、電壓等。
- **位置**：Linux kernel 子系統 `/sys/class/hwmon/`
- **目標**：統一各種硬體 sensor，讓使用者或應用程式可以用統一方式取得資料。

### 🔹 常見應用

- 伺服器 BMC 監控：溫度、風扇、電壓
- 開發板 / 嵌入式系統：系統健康檢查
- 自動控制：依照溫度自動調整風扇轉速
<br>

### **hwmon 的三個核心組件**

1. **核心驅動程式（Kernel Drivers）**
    - 這些驅動程式直接與硬體晶片溝通（如 I2C、SPI 或平台裝置）。
    - 會提供感測器資料給 hwmon 子系統。
    - 範例：`coretemp`（Intel CPU 溫度）、`thinkpad_acpi`（ThinkPad 筆電感測器）。
2. **Sysfs 介面**
    - Linux 核心透過 sysfs（虛擬檔案系統，通常掛載於 `/sys`）將感測器資料呈現給使用者空間。
    - 每個 hwmon 裝置會在 `/sys/class/hwmon/hwmonX/` 下生成對應節點。
    - 範例：讀 CPU 溫度：
        
        ```bash
        cat /sys/class/hwmon/hwmon0/temp1_input
        ```

3. **使用者空間工具**
    - 例如 `lm-sensors` 套件中的 `sensors` 命令。
    - 透過讀取 sysfs 節點取得感測器資料，並以可讀格式顯示給使用者
<br>

### Sysfs 介面
sysfs 介面是一個虛擬檔案系統，它為使用者空間應用程式提供了一種與核心驅動程式互動的方式  
以 hwmon 為例，它允許用戶空間應用程式讀取感測器數據並配置硬體監控晶片  
hwmon 裝置的 sysfs 條目通常位於 `/sys/class/hwmon/` 下  
可以透過 cat 或 echo 指令，像存取檔案一樣讀寫裝置的參數，不需要透過特殊的 ioctl 或 driver 專用介面  
<img width="563" height="330" alt="image" src="https://github.com/user-attachments/assets/adc4e1fd-54f0-4903-b964-fa97806691f6" />  
<br>

<img width="861" height="320" alt="image" src="https://github.com/user-attachments/assets/4ffa1a3d-8550-47d6-b0d3-267cdb1f223b" />  

  
### VIR0511H Driver 整體流程

```I2C 裝置上電
↓  
kernel I2C 子系統掃描 bus
↓  
匹配 driver
  ├─ 傳統 ID: i2c_device_id
  └─ Device Tree: of_device_id
↓  
呼叫 probe()
  ├─ 分配 driver 私有資料 (devm_kzalloc)
  ├─ 存 client 指標 (data->client)
  ├─ 初始化 mutex (data->update_lock)
  └─ 註冊 hwmon 裝置 (devm_hwmon_device_register_with_info)
↓  
使用者空間透過 sysfs 讀取
↓  
hwmon core 呼叫 driver 的 read() function
↓  
read() 會呼叫 vir0511h_update_client()
  ├─ 取回 driver 私有資料 (dev_get_drvdata)
  ├─ 檢查 cache 是否過期
  ├─ 如果過期，for each channel:
  │     ├─ 設定 ADC channel (vir0511h_set_config)
  │     ├─ 延遲等待 ADC 轉換 (mdelay)
  │     └─ 讀 ADC 值 (vir0511h_i2c_read_data)
  ├─ 更新 cache (data->temp_code[])
  └─ 更新 last_updated, valid flag
↓  
返回 clean / raw ADC value
  └─ read() 會清除 status bits ( & 0x7fff )
```


<br>

## 重要指令   
![messageImage_1765718993085](https://github.com/user-attachments/assets/9d4162f5-9ffc-4745-af13-41bb181a7980)
///////<img width="653" height="607" alt="image" src="https://github.com/user-attachments/assets/e6d8b810-f9b8-4921-a52c-4ad5f569c36e" />  
<br>

#### ✅`struct hwmon_chip_info`  
告訴 kernel 這個硬體監控晶片有哪些操作函式 (ops) 和感測器資訊 (info)
  - .ops : 指向前面定義的 fake_hwmon_ops，也就是告訴 kernel 如何讀取資料和控制顯示
  - .info : 指向 fake_info，描述晶片提供哪些感測器類型和屬性

作用：hwmon 子系統會根據這些資訊，幫你自動生成對應的 sysfs 屬性文件  
<br>


#### ✅`struct hwmon_ops`  
註冊一組 hwmon 操作函式 給 hwmon 子系統用  
  - 內容：
    - `read`、`write`、`update` 等函數指針。
    - 由 **hwmon core** 在 sysfs 存取時呼叫，完成與硬體的實際讀寫
    - is_visible → 控制 sysfs 權限，哪些通道可見、可讀、可寫
    - read → 提供讀取感測器值的 callback

作用：提供 hwmon 核心訪問 driver 的接口，例如讀取感測器值和控制 sysfs 權限     
<br>

#### ✅`struct hwmon_channel_info` 
描述一組「感測器通道」(channel) 的屬性和類型，告訴 kernel 該通道提供什麼類型的感測資料
  - 內容：
    - **監控類型**：如 `temp`、`fan`、`power`。
    - **支援的屬性**：如溫度感測器的各種屬性。
    - **通道號碼**：用於區分不同通道  

作用：hwmon 核心依據這個資訊，對每個通道自動生成對應的 sysfs 文件   
<br>

#### ✅`hwmon_dev = devm_hwmon_device_register_with_info(&client->dev, client->name, data, &vir0511h_chip_info, NULL);`  
註冊一個 hwmon 裝置到 sysfs
核心會自動在 /sys/class/hwmon/hwmonX/ 下建立對應節點
使用者空間透過 cat /sys/class/hwmon/hwmonX/temp1_input 讀取溫度
  - &client->dev：父 device，hwmon 裝置會掛在這個 device 下
  - client->name：裝置名稱，例如 "vir0511h"
  - data：driver 私有資料，hwmon core 會在 read/is_visible 裡傳回
  - &vir0511h_chip_info：前面定義的感測器 channel info + ops
  - NULL：選填，額外屬性或 callback，不常用
<br>

#### ✅`static int vir0511h_read(struct device *dev, enum hwmon_sensor_types type,u32 attr, int channel, long *val)` 
這是 hwmon 驅動的核心讀取函式，當使用者空間透過 sysfs 讀取溫度或其他感測器資料時會被呼叫。
目標：將快取的 raw ADC 值轉換成乾淨數值，回傳給使用者。
   - dev: Linux device model，對應 driver 的裝置
   - type: 感測器種類，例如 hwmon_temp 
   - attr: 感測器屬性，例如 hwmon_temp_input
   - channel: 感測器通道編號，對應 temp_code[channel]
   - val: 輸出變數，將讀到的值寫回使用者空間
<br>
#### ✅`static umode_t vir0511h_is_visible(const void *data, enum hwmon_sensor_types type, u32 attr, int channel)` 
控制 sysfs 權限與可見性
Linux hwmon core 會問這個函式：這個感測器的這個屬性、這個通道是否可讀？可寫？
   - data: driver 私有資料，通常不用修改
   - type: 感測器類型
   - attr: 屬性，例如 hwmon_temp_input
   - channel: 通道編號
<br>

#### ✅`static umode_t fake_is_visible(const void *data, enum hwmon_sensor_types type, u32 attr, int channel)`  
用來決定某個 hwmon attribute 是否要讓使用者看到，以及它的存取權限  
  - 感測器型態是溫度（hwmon_temp）
  - 屬性是 hwmon_temp_input（即 /sys/class/hwmon/hwmonX/temp1_input時，才允許它有讀取權限
<br>


#### ✅`PTR_ERR_OR_ZERO(hwmon_dev)`  
hwmon_device_register_with_info 回傳的是指標或錯誤編碼，用 PTR_ERR_OR_ZERO 宏轉換：
  - 如果是錯誤指標，回傳負錯誤碼
  - 如果是正常指標，回傳 0，表示成功
<br>

#### ✅`static struct platform_driver fake_driver`
告訴 Linux kernel 這是一個平台驅動，並且指定這個驅動的名字以及它的兩個重要回調函式（probe 與 remove）  
  - .driver : 一個 struct device_driver 結構
  - .name : 設定驅動的名字，名字會用來與 platform device 匹配
  - .probe : 是一個指向函式的指標，kernel 會呼叫這個函式去初始化設備
  - .remove :  也是一個函式指標，當設備被卸載或驅動被移除時呼叫





























































