## 簡單流程
 1. register_chrdev() → 註冊一個主編號
 2. class_create() → 建立 class，讓 udev 能感知裝置
 3. device_create() → 實際創建 /dev/mydev
 4. 寫 read/write/open/release → 讓裝置檔能被操作
<br>

## 重要指令  
mydev_class = class_create(THIS_MODULE, DEVICE_NAME)  
  - 建立一個 device class，會在 /sys/class/ 底下創建一個目錄，例如 /sys/class/mydev/  
  - 與 device_create() 搭配，讓 udev 根據這個 class 自動創建 /dev/mydev 裝置節點  
  - IS_ERR(mydev_class) :	判斷 class_create() 是否失敗。若失敗，mydev_class 會是一個錯誤指標  
  - return PTR_ERR(mydev_class) :回傳錯誤碼（轉換自錯誤指標），告知 kernel module 載入流程失敗  

mydev_device = device_create(mydev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME)  
  - 使用 class_create() 建立的裝置類別  
  - NULL :父裝置，通常為 NULL 表示沒有父裝置  
  - MKDEV(major, 0) :	設定這個裝置的主次編號（major/minor number），這裡次編號為 0  
  - NULL :裝置的私有資料，一般可設為 NULL  
  - DEVICE_NAME :裝置的名稱  
