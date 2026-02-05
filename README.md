# MemoryUtilityPkg
UEFI 函數使用筆記 (README)
本文件歸納了 Simple Text Input Protocol、Simple Text Output Protocol 以及 Boot Services 中關於事件與記憶體管理的關鍵函數。

1. 文字輸入 (Simple Text Input Protocol)
ReadKeyStroke()
讀取輸入設備的下一個按鍵輸入 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY) (
   IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
   OUT EFI_INPUT_KEY *Key
);
參數:


This: 指向 Protocol 實例的指標 。


Key: 指向 EFI_INPUT_KEY 結構的指標，用於存放按鍵資訊 。

說明:

若無按鍵輸入，函數會返回 EFI_NOT_READY 。

若有按鍵：


ScanCode: 對應 EFI 掃描碼 (如功能鍵) 。


UnicodeChar: 可列印字元；若按鍵無對應字元則為 0 。

回傳值:


EFI_SUCCESS: 成功讀取按鍵 。


EFI_NOT_READY: 目前無按鍵資料 。


EFI_DEVICE_ERROR: 硬體錯誤 。

2. 事件服務 (Boot Services - Events)
WaitForEvent()
暫停執行，直到指定的事件被觸發 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT) (
   IN UINTN NumberOfEvents,
   IN EFI_EVENT *Event,
   OUT UINTN *Index
);
參數:


NumberOfEvents: 事件陣列中的事件數量 。


Event: EFI_EVENT 陣列 。


Index: 指向滿足等待條件之事件索引值的指標 。

說明:

必須在 TPL_APPLICATION 優先級下呼叫，否則返回 EFI_UNSUPPORTED 。

依序評估陣列中的事件，直到有事件被觸發或發生錯誤 。

若事件處於 Signaled 狀態，會清除該狀態並返回成功 。

若要等待特定時間，需在陣列中包含 Timer 事件 。

回傳值:


EFI_SUCCESS: 指定索引的事件已被觸發 。


EFI_INVALID_PARAMETER: NumberOfEvents 為 0，或事件類型錯誤 。


EFI_UNSUPPORTED: 當前 TPL 不是 TPL_APPLICATION 。

3. 文字輸出 (Simple Text Output Protocol)
SetMode()
設定輸出設備的模式 。

原型 (Prototype):
typedef EFI_STATUS (* EFIAPI EFI_TEXT_SET_MODE) (
   IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
   IN UINTN ModeNumber
);
參數:


ModeNumber: 欲設定的文字模式編號 。

說明:

成功後，設備將切換至請求的幾何配置，並清除螢幕至當前背景色，游標歸零至 (0,0) 。

回傳值:


EFI_SUCCESS: 模式設定成功 。


EFI_DEVICE_ERROR: 設備錯誤 。


EFI_UNSUPPORTED: 模式編號無效 。

ClearScreen()
清除螢幕顯示 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
   IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);
說明:

使用當前背景色清除畫面 。

游標位置重置為 (0, 0) 。

回傳值:


EFI_SUCCESS: 操作成功 。

SetCursorPosition()
設定游標座標 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION) (
   IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
   IN UINTN Column,
   IN UINTN Row
);
參數:


Column, Row: 目標座標。必須在 QueryMode() 回傳的範圍內 。

說明:

螢幕左上角定義為 (0, 0) 。

回傳值:


EFI_SUCCESS: 成功 。


EFI_UNSUPPORTED: 游標位置無效 。

EnableCursor()
設定游標可見性 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR) (
   IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
   IN BOOLEAN Visible
);
參數:


Visible: TRUE 為顯示，FALSE 為隱藏 。

4. 記憶體管理 (Boot Services - Memory)
AllocatePages()
配置記憶體頁面 (Pages)
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES) (
   IN EFI_ALLOCATE_TYPE Type,
   IN EFI_MEMORY_TYPE MemoryType,
   IN UINTN Pages,
   IN OUT EFI_PHYSICAL_ADDRESS *Memory
);
參數:


Type: 配置類型 (如 AllocateAnyPages, AllocateAddress) 。


AllocateAnyPages: 配置任何滿足條件的頁面，忽略輸入的 Memory 地址 。


AllocateMaxAddress: 配置地址小於或等於輸入 Memory 的頁面 。


AllocateAddress: 配置在指定地址的頁面 。


MemoryType: 記憶體類型 (如 EfiLoaderData) 。UEFI App 通常使用 EfiLoaderData 。


Pages: 連續 4 KiB 頁面的數量 。


Memory: 輸入時依 Type 決定用途；輸出時為配置到的起始物理地址 。

回傳值:


EFI_SUCCESS: 成功 。


EFI_OUT_OF_RESOURCES: 無法配置 。


EFI_NOT_FOUND: 找不到符合請求的頁面 。

FreePages()
釋放記憶體頁面 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES) (
   IN EFI_PHYSICAL_ADDRESS Memory,
   IN UINTN Pages
);
參數:


Memory: 欲釋放頁面的起始物理地址 。


Pages: 欲釋放的連續 4 KiB 頁面數量 。

回傳值:


EFI_SUCCESS: 成功釋放 。


EFI_NOT_FOUND: 該記憶體未經 AllocatePages 配置 。

GetMemoryMap()
取得當前記憶體地圖 (Memory Map) 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP) (
   IN OUT UINTN *MemoryMapSize,
   OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
   OUT UINTN *MapKey,
   OUT UINTN *DescriptorSize,
   OUT UINT32 *DescriptorVersion
);
說明:

回傳系統中所有記憶體的描述符陣列，包含已配置與韌體使用的區域 。

OS 軟體需使用 DescriptorSize 來尋找陣列中下一個描述符的位置，以確保相容性 。


Buffer Too Small 處理: 若緩衝區太小，返回 EFI_BUFFER_TOO_SMALL，並在 MemoryMapSize 中提供所需大小 。再次呼叫時，建議配置比回傳值更大的緩衝區，因為配置動作本身可能會增加 Map 的大小 。


MapKey: 用於識別當前 Map 版本，ExitBootServices 時必須提供此 Key 。

AllocatePool()
配置記憶體池 (Pool) 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL) (
   IN EFI_MEMORY_TYPE PoolType,
   IN UINTN Size,
   OUT VOID **Buffer
);
參數:


PoolType: 記憶體池類型 。


Size: 欲配置的位元組 (Bytes) 數量 。


Buffer: 指向配置緩衝區指標的指標 。

說明:

所有配置皆為 8-byte 對齊 。

回傳值:


EFI_SUCCESS: 成功 。


EFI_OUT_OF_RESOURCES: 無法配置 。

FreePool()
釋放記憶體池 。

原型 (Prototype):
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL) (
   IN VOID *Buffer
);
參數:


Buffer: 指向欲釋放緩衝區的指標 。

說明:

釋放後，該記憶體類型將變回 EfiConventionalMemory 。

回傳值:


EFI_SUCCESS: 成功釋放 。


EFI_INVALID_PARAMETER: Buffer 無效 。

__________________________________________________________
cd /d D:\BIOS\MyWorkSpace\edk2

edksetup.bat Rebuild

chcp 65001

set PYTHONUTF8=1

set PYTHONIOENCODING=utf-8

rmdir /s /q Build\MemoryUtilityPkg

build -p MemoryUtilityPkg\MemoryUtilityPkg.dsc -a X64 -t VS2019 -b DEBUG
