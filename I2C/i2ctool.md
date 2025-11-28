

## 🔹 什麼是 i2c-tools
`i2c-tools` 是 Linux 下的一套使用者空間工具，用來操作和測試 I²C 裝置。  
它提供直接對 I²C bus 的存取，方便開發者驗證硬體、測試驅動或進行快速 debug。

### 工具列表

- `i2cdetect` → 掃描 bus 上有哪些 slave address
- `i2cdump` → 讀取裝置暫存器/EEPROM 的完整內容
- `i2cget` → 讀單一暫存器
- `i2cset` → 寫單一暫存器
- `i2ctransfer` → 模擬完整 I²C 傳輸序列（進階）

這些就是 **日常 debug I²C 最快的工具**   
<br><br>

## ✅i2cdetect

- **用途**：掃描 I²C bus 上有哪些裝置存在，輸出成一張 **地址表**。
- **輸入**：指定 bus 編號（例如 `/dev/i2c-1` → bus=1）。
- **輸出**：
    - `-` → 沒有回應
    - `UU` → 地址被驅動程式佔用（通常確實有裝置）
    - `0xXX` → 偵測到裝置的地址

### 📌 常用選項

- `y` → 不要互動確認，直接掃描（方便用在 script）。
- `F` → 顯示該 bus 支援的功能（像是 SMBus Read/Write）。
- `l` → 列出所有可用的 I²C bus。
- `first last` → 限定掃描的地址範圍（預設是 0x03 ~ 0x77）。

**i2cdetect -y 3  //掃描bus下設備 3指bus number**

⚠️ **警告**：
`i2cdetect` 掃描會發送探測指令，可能會干擾某些敏感的 I²C 裝置（EEPROM、時鐘晶片），導致 **bus 掛死或數據錯亂**。所以工程師在真實系統上用它要很小心，建議先確認哪些地址安全   
<br><br>

## ✅i2cdump 

- **用途**：用來 **查看 I²C 裝置的暫存器內容**，方便工程師 debug 或驗證裝置狀態。
- **輸入**：
    - `i2cbus` → 指定要操作的 I²C bus（對應 `/dev/i2c-*`）
    - `address` → 指定裝置的 I²C 位址
    - `mode` → 指定讀取方式（byte, word, SMBus block, I2C block, EEPROM c/W 模式）
    - `bank` / `bankreg` → 某些多 bank 晶片（如 W83781D）用來選擇暫存器 bank
---  
### 📌 常用選項

| 選項 | 功能 | 注意事項 |
| --- | --- | --- |
| `-y` | 不互動，直接執行 | script 使用方便 |
| `-f` | 強制存取已被 kernel driver 控制的裝置 | 危險，可能導致 driver 與硬體異常或讀取錯誤 |
| `-r first-last` | 限定讀取的暫存器範圍 | 依 mode 不同，限制也不同 |
| `-V` | 顯示版本 | - |

### 📌 模式說明  
| mode | 說明 | 適用範例 |
| --- | --- | --- |
| `b` | 單 byte | 讀 8-bit 寄存器 |
| `w` | 16-bit word | 讀 16-bit 寄存器 |
| `s` | SMBus block | 讀 SMBus 區塊資料 |
| `i` | I²C block | 讀 I²C 區塊資料 |
| `c` | 連續讀，address auto-increment | EEPROM 類晶片 |
| `W` | 類似 word，但只在偶數寄存器讀 | EEPROM 特殊需求 |
| `p` | 可附加 PEC（Packet Error Check） | 提高通訊可靠性 |

### ⚠️ 風險  
- `c` 模式會先 **寫入一個 byte** 到 chip 的地址 pointer register，部分晶片可能會當作真正寫入，導致資料被覆蓋或裝置異常。
- **不建議隨意對陌生地址使用 i2cdump**，要先了解裝置 datasheet。
- 使用 `f` 強制存取已被 kernel driver 控制的裝置，也會造成 driver 與硬體衝突
<br><br>


## ✅ i2cget 

- **用途**：從 I²C/SMBus 裝置讀取單一暫存器的內容
- **輸入**：
    - `i2cbus` → I²C bus 編號（如 `/dev/i2c-1`）
    - `chip-address` → 裝置位址 (0x03~0x77)
    - `data-address` → 要讀取的暫存器位址 (0x00~0xFF)，可省略
    - `mode` → 讀取方式：
        - `b` → 讀 1 byte
        - `w` → 讀 16-bit word
        - `c` → write byte / read byte transaction
        - `p` → 可附加 PEC
---
### 📌 常用選項

| 選項 | 功能 | 注意事項 |
| --- | --- | --- |
| `-y` | 不要互動，直接執行 | script 使用方便 |
| `-f` | 強制存取已被 kernel driver 控制的裝置 | 危險，可能造成 driver 與硬體異常 |

### ⚠️ 風險
- 某些晶片把 **SMBus 讀取** 看成寫入操作，可能會改變暫存器或狀態
- `cp` 模式（write byte/read byte with PEC）尤其危險
- 不建議隨意對陌生地址使用
<br><br>

## ✅ i2cset 

- **用途**：寫入 I²C 裝置暫存器，設定裝置參數
- **輸入**：
    - `i2cbus` → I²C bus 編號
    - `chip-address` → 裝置位址 (0x03~0x77)
    - `data-address` → 暫存器位址 (0x00~0xFF)
    - `value` → 要寫入的數值（可多個，用於 block / word）
    - `mode` → 寫入方式：
        - `b` → 寫 1 byte
        - `w` → 寫 16-bit word
        - `s` → SMBus block write
        - `i` → I²C block write
        - `c` → short write (只改 address pointer，不寫資料)
        - `p` → 附加 PEC
---
### 📌 常用選項

| 選項 | 功能 | 注意事項 |
| --- | --- | --- |
| `-y` | 不互動確認，直接寫 | script 使用方便 |
| `-f` | 強制寫入，即使被 kernel driver 控制 | 可能破壞裝置或 driver |
| `-m mask` | 只寫 value 指定的 bits，其他 bits 保留 | 需確認硬體支援，否則結果不可靠 |
| `-r` | 寫完立即讀回比對 | 用於驗證寫入是否成功 |

### ⚠️ 風險

- `i2cset` **比 i2cget / i2cdump 危險**，可能：
    - 干擾 I²C bus
    - 改錯暫存器 → 造成裝置異常
    - 寫入 EEPROM 可能 **破壞記憶體模組** → 系統無法開機
- **使用時務必確認 datasheet**，尤其是生產板或客戶環境
<br>

## ✅ i2ctransfer

- **用途**：在同一條 I²C bus 上，執行多種 **複雜的讀寫序列**
- **特色**：
    - 可以一次操作多個暫存器 / 多個值
    - 可組合 **write + read + write + read** 等多段交易
    - 適合模擬 driver 或硬體操作流程

**基本語法**

```bash
i2ctransfer [-f] [-y] [-v] I2CBUS DESC [DATA] [DESC [DATA]]...
```

- `I2CBUS` → bus 編號或名稱
- `DESC` → 描述交易：
    - `rLENGTH[@address]` → 讀 LENGTH bytes
    - `wLENGTH[@address]` → 寫 LENGTH bytes
- `DATA` → 寫入的 bytes（可用 `=` / `+` /  / `p` 簡化）
- 選項：
    - `y` → 不互動確認
    - `f` → 強制存取，即使 driver 控制該裝置
    - `v` → 顯示更詳細資訊
### 寫入多個 byte

```bash
# 往 i2c bus 4 上 0x30 裝置寫入 3 bytes 到寄存器 0x85
i2ctransfer -y -f 4 w3@0x30 0x85 0x01 0x10
```

- `w3@0x30` → 寫入 3 bytes
- 後面跟的 `0x85 0x01 0x10` → 寫入的資料

---

### 從某寄存器讀取多個 byte

```bash
# 從 i2c bus 4 上 0x30 裝置的 0x8501 寄存器讀取 3 bytes
i2ctransfer -y -f 4 w2@0x30 0x85 0x01 r3
```

- `w2@0x30 0x85 0x01` → 先寫入寄存器地址 (0x8501)
- `r3` → 再讀 3 bytes

> 這裡的核心概念跟 i2cget -c 一樣：先寫入暫存器位址，再讀取資料


