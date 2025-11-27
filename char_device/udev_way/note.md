## 簡單流程
**1. alloc_chrdev_region()     → 取得 major/minor**  
**2. cdev_init()               → 初始化 cdev，綁定 fops**  
**3. cdev_add()                → 將 cdev 註冊到 kernel**  
**4. class_create()            → 註冊到 /sys/class**  
**5. device_create()           → 由 udev 自動建立 /dev 節點** 
<br>

## 重要指令  

### ✅`alloc_chrdev_region()`


**用途：向 Kernel 申請一組全新的 Major / Minor 編號**

#### 功能

動態向 kernel 申請字元裝置的編號：

```c
int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);
```
它會分配：

- **major number（主編號）**
- **count 個 minor number（次編號）**

成功後，你會得到：

```c
MAJOR(*dev);  // 取得 major
MINOR(*dev);  // 取得 minor
```

#### 什麼時候用？

當你需要：
- **使用新的裝置節點（/dev/xxx）**
- **自己管理 cdev 結構**
- **符合現代 kernel driver 寫法**  

### ✅`cdev_init()`

**用途：初始化 cdev 結構，綁定 file_operations**  
```c
struct cdev my_cdev;

cdev_init(&my_cdev, &fops);
my_cdev.owner = THIS_MODULE;
```

`cdev` 是 kernel 記錄 "一個字元裝置" 的內部物件。

你在使用者空間呼叫 open/read/write，就是透過這個 `cdev` 路由進來的  

它做的事：

- 註冊 file_operations
- 初始化內部資料結構
- 尚未加入系統（**還不可被使用**）


### ✅`cdev_add()`

**用途：把已初始化的 cdev 正式加入系統，讓 /dev 可以使用**  
```
cdev_add(&my_cdev, dev, count);
```  

做了什麼？

- 將 cdev 與 major/minor 編號綁定
- 正式讓 kernel 接管，成為可用的字元裝置
- 此時裝置已能支援 open/read/write  


### ✅`mydev_class = class_create(THIS_MODULE, DEVICE_NAME)`  
- 在 **sysfs** (`/sys/class/`) 裡建立一個 **class** 目錄，例如：
    
    ```swift
    /sys/class/mydev/
    ```
    
- 這個 class 是一種「裝置分類」，讓 **udev** 知道當有這個 class 的裝置出現時，要幫你自動創建對應的 `/dev/` 節點。
- 失敗處理：
    - 如果建立失敗，會回傳 **錯誤指標**。
    - `IS_ERR(mydev_class)`：判斷是否錯誤。
    - `PTR_ERR(mydev_class)`：把錯誤指標轉換成「錯誤碼」傳回，告訴 kernel module 初始化失敗   


### ✅`mydev_device = device_create(mydev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME)`  
- 在 sysfs 內的 class 目錄底下，創建一個「裝置項目」：
    
    ```swift
    /sys/class/mydev/mydev/
    ```
    
- 參數解釋：
    1. **`mydev_class`**：剛剛用 `class_create` 建立的 class。
    2. **`NULL`**：父裝置（沒有父裝置時用 NULL）。
    3. **`MKDEV(major, 0)`**：組合出 **主設備號 (major)** + **次設備號 (minor)**。
    4. **`NULL`**：私有資料（通常不用）。
    5. **`DEVICE_NAME`**：裝置名稱 → `/dev/mydev` 就是這樣得來的。
- 一旦呼叫 `device_create()` 成功：
    - **sysfs** 會新增：
        
      ```
      /sys/class/mydev/mydev
      ```
        

**udev** 監聽到這個事件，就會自動建立：
  ```
  /dev/mydev
  ```
