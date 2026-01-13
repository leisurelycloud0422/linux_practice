## 目錄
- **Linux 裡的 Device**
    - **Character Device**
    - **Block Device**
- **/dev/mydev**
- **cdev**
    - **register_chrdev**
    - **alloc + cdev_init + cdev_add**
- **file_operations**
- **重要指令** 
<br><br>

## Linux 裡的 Device（裝置）

在 Linux 裡，**裝置（Device）** 是用來跟硬體互動的抽象介面。

Linux 採用「**一切皆檔案**」的設計，把裝置當成一種檔案（裝置檔），放在 `/dev/` 目錄下，例如：

- `/dev/tty` → 終端機
- `/dev/sda` → 硬碟

這些裝置大致可以分為 **兩大類**：

## 1. 字元裝置（Character Device）

- **傳輸方式**
    - 一次讀寫一個「字元（byte）」或一串位元組
    - 資料是線性傳輸、按順序流動的
**特點**

- **不支援隨機存取** → 不能像檔案一樣「seek 到某個位置」
- 適合 **資料量小、連續傳輸** 的應用
- 範例：鍵盤輸入資料是「一個一個字元」送進來的

## 2. 區塊裝置（Block Device）

- **傳輸方式**
    - 以「區塊（block）」為單位（通常 512 bytes 或 4KB）
    - 支援 **隨機存取（random access）**
- **特點**
    - 支援 **檔案系統（ext4、FAT...）**
    - 適合 **大量資料** 或 **隨機存取** 的應用
<br>
 

## `/dev/mydev` 是什麼？

- `/dev/mydev` 是一個 **裝置節點（device node）**，又稱 **特殊檔案**。
- 它是使用者程式與核心驅動程式之間的 **橋樑**。
- 透過它，使用者可以用 `cat`、`echo`、`read`、`write` 等方式操作硬體，而不用直接接觸硬體寄存器。

> 可以把它想像成「驅動程式的入口門戶」
<br>


## cdev

在 Linux 中，cdev（character device object）是 **VFS 與 driver 之間的真正橋梁**。

要讓 `/dev/mydev` 能呼叫到 `open/read/write`，就需要建立 cdev。

建立 cdev 也有 **兩種方式**：

### 方法一：舊式建立方式（使用 register_chrdev）

#### ✔ 不需要手動創建 cdev

`register_chrdev()` 會：

- 分配 major number
- **自動內部建立一個 cdev**
- 將 file_operations 綁定上去

#### ✔ 缺點

- 無法指定 minor number（只支援一個 minor=0）
- 不好管理
- 不推薦在嵌入式或產品使用（已被視為 legacy）
<br>

### 方法二：現代標準流程（alloc + cdev_init + cdev_add）

#### ✔ 需要手動建立 cdev

現代 Linux driver 的標準方法：

1. **alloc_chrdev_region()**
    
    → 分配 major/minor number
    
2. **cdev_init()**
    
    → 初始化 cdev object，將 file_operations 綁上
    
3. **cdev_add()**
    
    → 正式把 cdev 註冊到 kernel（讓 VFS 可以操作）
    

#### ✔ 優點

- 多個 minor（支援多裝置）
- 完全控制 lifecycle
- 所有正式 kernel driver 都使用這方式

| 功能 | register_chrdev() | alloc_chrdev_region() + cdev_init() + cdev_add()（現代寫法） |
| --- | --- | --- |
| major/minor | **只能申請一個 major、所有 minor** | 動態申請 major，**可以選擇使用多少 minor** |
| cdev 管理 | kernel 自動建立匿名 cdev，**無法管理** | 自己建立 cdev，**完全控制** |
| 支援多裝置 | 很難（固定一個 major） | 可以管理多個 minor，非常適合多裝置 driver |
| udev 整合 | 較舊、不推薦 | 與 device_create()/class_create() 搭配最常用 |
| 是否過時 | **老舊 API，不推薦** | **目前 kernel 官方推薦使用** |  
<br>


## `struct file_operations`   
- `struct file_operations`（簡稱 **fops**）是 Linux 核心提供的一個結構，用來描述 **裝置（device）可支援的操作**。
- 每個字元裝置或區塊裝置都可以透過它，讓核心知道使用者程式對 `/dev/<device>` 做的操作要呼叫哪個函式。

```c
struct file_operations fops = {
    .open = my_open,
    .release = my_release,
    .read = mydev_read,
    .write = mydev_write,
};

```

把 4 個行為註冊進 kernel。
**每次使用者對 `/dev/mydev` 做 I/O 時，kernel 就會呼叫實作的這些函式。**  
####  1. open()
此範例只簡單印訊息, 但一般 driver 會做：

✔ 初始化 private_data

✔ 分配暫存 buffer

✔ 計數使用者數量

✔ 初始化硬體（I2C device、GPIO、SPI 等）

####  2. release()
通常做：

✔ 回收資源

✔ free 私有資料

✔ 把裝置設定回安全狀態

✔ 減少使用者引用計數（例如用在多開控制）  
<br>

## 重要指令
### `copy_to_user()` — 讀資料給使用者

```
copy_to_user(buf, device_buffer + *ppos, count);
```

- 將 `device_buffer` 的資料複製到使用者提供的 `buf`
- 從 `ppos` 偏移開始，複製 `count` 個 byte
- 常用於 `read()` 函式
<br>


### `copy_from_user()` — 從使用者取得資料

```
copy_from_user(device_buffer, user_buf, count);
```

- 將使用者提供的 `user_buf` 複製到 kernel 的 `device_buffer`
- 常用於 `write()` 函式
<br>


### `register_chrdev()` — 註冊字元裝置

#### 原型

```c
int register_chrdev(unsigned int major, const char *name, const struct file_operations *fops);
```

#### 功能

- 註冊一個 **簡單的字元裝置**，不需要建立 device class 或使用 udev
- 讓核心知道這個字元裝置的 **主設備號 (major number)** 與對應的 **操作函式表 (`file_operations`)**
<br>

### `unregister_chrdev()` — 移除字元裝置

#### 原型

```
void unregister_chrdev(unsigned int major, const char *name);
```

#### 功能

- 移除先前用 `register_chrdev()` 註冊的字元裝置
- 釋放 **主設備號 (major number)**，避免資源洩漏
- `name` 必須與註冊時使用的名稱相同

 **重點**：

- 使用 `major=0` 可讓核心自動分配空閒的 major number
- 註冊成功後，可以用 mknod 建立 `/dev/<name>` 進行操作
- 卸載模組或結束使用時，一定要呼叫 `unregister_chrdev()`
<br>



### `sudo mknod /dev/mydev c 240 0`  
```bash
# 假設 major 是 240，創建裝置節點
sudo mknod /dev/mydev c 240 0
```

- **mknod**：用來在 `/dev` 下手動創建一個設備節點 (device file)。
- `/dev/mydev`：裝置檔案的名稱。
- **c**：表示這是 **字元裝置 (character device)**。
- `240`：這是 **主設備號 (major number)**，它告訴 Linux 這個裝置要交給哪個驅動程式處理。
- `0`：這是 **次設備號 (minor number)**，通常用來區分同一類裝置下的不同實例。














