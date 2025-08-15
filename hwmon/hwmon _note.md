## hwmon 子系統
hwmon 子系統由幾個關鍵組件組成： 
- 核心驅動程式：這些驅動程式與硬體監控晶片接口，並將感測器資料公開給用戶空間  
- Sysfs 介面：一個虛擬檔案系統，允許用戶空間應用程式與核心驅動程式互動並讀取感測器資料  
- 使用者空間工具：來自 lm-sensors 套件的「感測器」等工具從 sysfs 介面讀取感測器資料並將其顯示給使用者

hwmon是 Linux 的一個子系統，用來統一處理來自硬體感測器的資訊，例如：  
溫度（temp） 
電流（curr）  
風扇轉速（fan）   
功率（power）  
這些資訊會透過 sysfs（一種虛擬檔案系統）呈現在使用者空間  
<br>

### Sysfs 介面
sysfs 介面是一個虛擬檔案系統，它為使用者空間應用程式提供了一種與核心驅動程式互動的方式  
以 hwmon 為例，它允許用戶空間應用程式讀取感測器數據並配置硬體監控晶片  
hwmon 裝置的 sysfs 條目通常位於 `/sys/class/hwmon/` 下  
可以透過 cat 或 echo 指令，像存取檔案一樣讀寫裝置的參數，不需要透過特殊的 ioctl 或 driver 專用介面  
<img width="563" height="330" alt="image" src="https://github.com/user-attachments/assets/adc4e1fd-54f0-4903-b964-fa97806691f6" />  
<br>

<img width="861" height="320" alt="image" src="https://github.com/user-attachments/assets/4ffa1a3d-8550-47d6-b0d3-267cdb1f223b" />  

## 整體流程  
1. 撰寫 hwmon 驅動程式
2. 定義 chip 支援哪些感測器（temp1_input）
3. 實作讀取函數（.read callback）
4. 使用 hwmon_device_register_with_info() 註冊裝置
5. Kernel 自動建立 /sys/class/hwmon/hwmon*/temp1_input 節點
6. 使用者透過 cat 指令查看溫度值
<br>

## 重要指令   
<img width="653" height="607" alt="image" src="https://github.com/user-attachments/assets/e6d8b810-f9b8-4921-a52c-4ad5f569c36e" />  
<br>

#### static int fake_temp_read(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)  
自訂模擬用的，所以參數沒有被實際使用，不過在真實的 hwmon driver 裡很重要
   - dev: 裝置指標
   - type: 感測器類型（這裡是 hwmon_temp）
   - attr: 屬性類型（這裡會是 hwmon_temp_input）
   - channel: 第幾個感測通道（temp1、temp2 等，這裡只有 channel 0）
   - val: 用來回傳讀到的資料
<br>

#### static umode_t fake_is_visible(const void *data, enum hwmon_sensor_types type, u32 attr, int channel)  
用來決定某個 hwmon attribute 是否要讓使用者看到，以及它的存取權限  
  - 感測器型態是溫度（hwmon_temp）
  - 屬性是 hwmon_temp_input（即 /sys/class/hwmon/hwmonX/temp1_input時，才允許它有讀取權限
<br>

#### static const struct hwmon_ops fake_hwmon_ops  
註冊一組 hwmon 操作函式 給 hwmon 子系統用  
  - .is_visible : fake_is_visible 決定 sysfs 中有哪些屬性會被顯示，以及它們的權限  
  - .read : 實作讀取某個 sensor 值的函式
<br>

#### static const struct hwmon_channel_info *fake_info[] 
描述一組「感測器通道」(channel) 的屬性和類型，告訴 kernel 該通道提供什麼類型的感測資料
  - 這裡用 HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT) 巨集表示
  - 感測器類型是「溫度」（temp）
  - 屬性是「輸入值」（HWMON_T_INPUT）
  - 陣列最後放 NULL 是為了表示結束
<br>

#### static const struct hwmon_chip_info fake_chip_info  
告訴 kernel 這個硬體監控晶片有哪些操作函式 (ops) 和感測器資訊 (info)
  - .ops : 指向前面定義的 fake_hwmon_ops，也就是告訴 kernel 如何讀取資料和控制顯示
  - .info : 指向 fake_info，描述晶片提供哪些感測器類型和屬性
<br>

#### hwmon_device_register_with_info(&pdev->dev, "fake_temp_sensor", NULL, &fake_chip_info, NULL)  
將這個 device 註冊到 hwmon 子系統中
  - &pdev->dev：傳入設備本身的 struct device
  - "fake_temp_sensor"：指定在 sysfs 下的名稱，會成為 /sys/class/hwmon/hwmonX/name 的內容
  - NULL：傳入的私有資料指標，這裡不使用
  - &fake_chip_info：之前定義的 hwmon_chip_info，提供感測器通道和操作函式資訊
  - NULL：保留參數，通常用不到
<br>

#### PTR_ERR_OR_ZERO(hwmon_dev)  
hwmon_device_register_with_info 回傳的是指標或錯誤編碼，用 PTR_ERR_OR_ZERO 宏轉換：
  - 如果是錯誤指標，回傳負錯誤碼
  - 如果是正常指標，回傳 0，表示成功
<br>

#### static struct platform_driver fake_driver
告訴 Linux kernel 這是一個平台驅動，並且指定這個驅動的名字以及它的兩個重要回調函式（probe 與 remove）  
  - .driver : 一個 struct device_driver 結構
  - .name : 設定驅動的名字，名字會用來與 platform device 匹配
  - .probe : 是一個指向函式的指標，kernel 會呼叫這個函式去初始化設備
  - .remove :  也是一個函式指標，當設備被卸載或驅動被移除時呼叫





























































