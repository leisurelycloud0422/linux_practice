## Device
在 Linux 裡，Device是用來跟硬體互動的抽象介面。Linux 會把裝置當成一種檔案（裝置檔），例如 /dev/tty、/dev/sda。這些裝置大致可以分為兩類  
#### 1.字元裝置（Character Device）  
  傳輸方式 :一次讀寫一個「字元（byte）」或一串位元組。資料是線性傳輸、按順序流動的  
  常見範例 ：鍵盤、滑鼠、序列埠（serial port，例如 /dev/ttyS0）、GPIO 控制器  
  開發方式 ：透過 Linux 核心提供的 file_operations 結構，實作 read()、write()、open() 等函式來操作裝置  
  特點     ：資料不支援隨機存取（不能「跳到某一個位置」讀資料），適合資料量小、單向、連續的輸入/輸出  
#### 2.區塊裝置（Block Device）
  傳輸方式 ：以「區塊（block）」為單位，一次讀寫一整塊資料（通常是 512 bytes 或 4KB）。資料可以任意存取（random access）  
  常見範例 ：硬碟（HDD / SSD，例如 /dev/sda）、記憶卡、USB 儲存裝置、RAM disk  
  開發方式 ：透過較複雜的 API（如 block_device_operations），配合系統的cache 機制進行資料交換  
  特點 ：支援檔案系統（ext4、FAT 等），適合大量資料讀寫與隨機存取應用  
<br><br>
## /dev/mydev
在 Linux 裡，/dev/mydev 是驅動的「裝置節點」（device node）—— 也就是一個「特殊檔案」，讓使用者程式（像 cat、echo）可以與核心模組互動  

這個節點本質上是一個「指向驅動程式的入口」  

### 怎麼創建 /dev/mydev？
##### - 方法一：手動建立（使用 mknod）
用 register_chrdev() 或 register_chrdev_region() 註冊設備後，系統會給一個主設備號（major number），可以根據這個號碼手動建立節點  
mknod	: 手動創建裝置節點，需提供 major/minor
major/minor	: 驅動與裝置的對應編號
##### - 方法二：自動建立（使用 udev + class_create）
可以透過 class_create() 搭配 device_create()，讓核心主動通知 udev（使用者空間的裝置管理守護程式），自動建立對應的/dev/裝置名稱 節點  
class_create()：創建一個類別（class），代表一組裝置  
device_create()：創建一個裝置，udev 會偵測到並自動建立 /dev/mydev  
<br><br>
## struct file_operations
Linux 核心裡用來定義「這個裝置可以做哪些操作」的結構體  
當使用者對 /dev/裝置 做出動作（如 cat、echo），系統會呼叫這個結構裡對應的函式  

<br><br>
<img width="598" height="185" alt="image" src="https://github.com/user-attachments/assets/f726037c-7a70-4350-96fd-6c4c58226bcb" />

-  .owner :設定為 THIS_MODULE，代表這個結構是這個模組的一部分。防止模組在使用中被卸載  
-  .open :裝置被開啟時呼叫，例如 open("/dev/mydev")  
-  .release : 裝置被關閉時呼叫，例如 close(fd)  
-  .read : 裝置被讀取時呼叫，例如 cat /dev/mydev  
-  .write : 裝置被寫入時呼叫，例如 echo hello > /dev/mydev
<br><br>



## 重要指令
#### static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) 
  - file :使用者對裝置開啟的檔案描述結構
  - buf	:使用者提供的 buffer，要把資料讀進去（user space）
  - count :使用者想讀多少 byte
  - ppos :指向「目前讀取位置」的偏移值指標（位移指標）
  - __user ：Linux 核心自己定義的macro,標註「這個指標是指向 user-space 的資料」
  - loff_t :表示檔案位移的資料型別，64 位元整數，支援超過 2GB 檔案的位移
  - size_t :無號整數 (ssize_t 是有號整數)  

#### copy_to_user(buf, device_buffer + *ppos, count)  
  - copy_to_user() 將kernel space的資料複製到user space的 buf
  - 從 device_buffer + *ppos 開始複製 count 個 byte

#### copy_from_user(device_buffer, user_buf, count)
  - 將user space的資料複製到kernel space的 buf
<img width="478" height="284" alt="image" src="https://github.com/user-attachments/assets/c9bef77d-4d52-49fd-b82c-a1e0ceb0480a" />  

#### int register_chrdev(unsigned int major, const char *name, const struct file_operations *fops)   
  - 註冊一個簡單的字元裝置（無需 device class）
  - major傳入 0 表示：請核心自動分配一個空的主設備號（major number）
  - DEVICE_NAME：是這個設備的名稱
  - &fops指向一個 struct file_operations 結構(open/read/write）
  - 回傳值：≥0：代表成功，並且是分配到的 major number 。 <0：代表失敗，會是一個錯誤碼（例如 -EBUSY, -ENOMEM）

#### void unregister_chrdev(unsigned int major, const char *name)  
  - 移除先前用 register_chrdev() 註冊的字元裝置，釋放主設備號，避免資源洩漏
  - 字元裝置名稱，需與 register_chrdev() 使用的相同

#### sudo mknod /dev/mydev c 240 0 (mknod [路徑] [類型] [主設備號] [次設備號])  
  - /dev/mydev：建立的裝置檔案名稱（裝置節點會出現在 /dev/ 下）
  - c :代表是 字元裝置（char device），不是 block 裝置
  - 240 ：major number，用來指定這個裝置要交給哪個驅動處理（模組裡 register_chrdev() 回傳的數字）
  - 0 ：minor number，通常用來區分同一類裝置的不同實例，這裡設為 0 即可

#### dmesg | grep mydev (查看 major number)




