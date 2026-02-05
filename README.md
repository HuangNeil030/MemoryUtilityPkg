# MemoryUtilityPkg
1. 這個工具能做什麼

本工具提供兩大功能頁：

Dump Memory Map

讀取 UEFI Boot Services 的 Memory Map

逐段列出：Memory Type / 起始位址 / 結束位址 / Pages

統計每種 type 的 pages，並估算總記憶體大小（MB）

Allocate Memory

Allocate Pages：使用 AllocatePages() 分配頁（Page = 4KB）

Allocate Pools：使用 AllocatePool() 分配位元組（Byte）

分配完成可進入 Hex Editor 檢視/修改記憶體內容

最後可選擇釋放：FreePages() / FreePool()
2. 操作方式總覽（按鍵與輸入規則）
2.1 Menu 選單頁（主選單/子選單/MemoryType 選單）

↑ / ↓：移動選取

Enter：確認

ESC：返回上一層 / 取消

2.2 數字輸入規則（PromptU64）

支援兩種格式：

十進位：4096

十六進位：0x100000

輸入時支援：

Backspace：刪字

Enter：送出

ESC：取消（回傳 EFI_ABORTED）

2.3 Hex Editor（16x16 顯示）

← / → / ↑ / ↓：移動游標（每格 1 byte）

PgUp / PgDn：翻頁（每頁 256 bytes）

Enter：修改游標所在 byte（輸入 hex byte）

ESC：離開 Hex Editor
3. 功能 1：Dump Memory Map
3.1 使用流程

主選單選 Dump Memory Map

工具會呼叫 gBS->GetMemoryMap() 取得 memory descriptors

顯示列表（每 20 行暫停一次）

最後顯示各 type pages 統計 + Total Memory (MB)

任意鍵返回主選單

3.2 背後的關鍵 API：GetMemoryMap（兩段式）

第一次呼叫：MapSize=0, Map=NULL

預期回傳：EFI_BUFFER_TOO_SMALL

這次會把「需要的 MapSize」寫回給你

AllocatePool：用取得的 MapSize 去分配 buffer

第二次呼叫：真正拿到 Memory Map

3.3 常見錯誤碼與原因

EFI_BUFFER_TOO_SMALL

第一次呼叫預期會發生（這是正常流程）

EFI_OUT_OF_RESOURCES

AllocatePool 失敗，記憶體不足或環境限制

EFI_INVALID_PARAMETER

參數不合法（通常是你傳錯指標或 MapSize）

3.4 你可以怎麼擴充

顯示 D->Attribute（例如 UC/WC/WB/XP…）

顯示 VirtualStart（在切換虛擬位址後才重要）

支援搜尋/過濾 MemoryType（只看 Conventional / ACPI…）

4. 功能 2：Allocate Pages
4.1 使用流程（UI）

主選單：Allocate Memory

子選單：選 Allocate Pages

選 Memory Type（例如 EfiBootServicesData）

選 Allocate Type

AllocateAnyPages

AllocateMaxAddress（需要輸入上限地址）

AllocateAddress（需要輸入固定地址）

輸入 pages 數量

顯示成功後的範圍：Start ~ End

問你要不要進 Hex Editor（y/n）

問你要不要 FreePages（y/n）

4.2 背後 API

gBS->AllocatePages(AllocateType, MemoryType, Pages, &Addr)

gBS->FreePages(Addr, Pages)

4.3 Pages、Bytes、地址範圍怎麼算

1 page = 4096 bytes

TotalBytes = EFI_PAGES_TO_SIZE(Pages)

End = Start + TotalBytes - 1

4.4 常見錯誤碼與原因

EFI_OUT_OF_RESOURCES

沒有足夠的連續頁面可分配

EFI_INVALID_PARAMETER

AllocateType/MemoryType 不合法

AllocateAddress 但地址不是 page-aligned（通常必須 4KB 對齊）

EFI_NOT_FOUND（較少）

某些平台在 MaxAddress 模式找不到可用區間

4.5 你可以怎麼擴充

讓使用者輸入「bytes」，你幫他換算 pages（bytes→pages 向上取整）

AllocatePages 後自動 SetMem() 清零或填固定 pattern

增加「顯示當前游標的全域地址」：Addr + BaseOffset + Cursor

5. 功能 3：Allocate Pools（你要求的限制版）
5.1 你這版的核心規則（非常重要）

使用者輸入：Bytes（想要的大小，也是可寫範圍）

實際分配：AllocSize = max(Bytes, 256)

目的：畫面永遠能顯示 16x16 = 256 bytes

Hex Editor：

顯示範圍：看 AllocSize

可修改範圍：只允許 0 .. Bytes-1

超出範圍按 Enter 會提示：Out of allocated range

5.2 使用流程（UI）

Allocate Memory → Allocate Pools

選 Memory Type

輸入 Bytes（你真正需要的大小）

顯示：

Requested: Bytes

Allocated: AllocSize (>=256)

Address Range（印 requested 範圍）

進 Hex Editor（可選）

FreePool（可選）

5.3 背後 API

gBS->AllocatePool(MemoryType, AllocSize, &Buf)

gBS->FreePool(Buf)

5.4 常見錯誤碼與原因

EFI_OUT_OF_RESOURCES

pool 分配失敗（資源不足）

EFI_INVALID_PARAMETER

MemoryType 不合法，或指標/大小不對

EFI_ACCESS_DENIED（少見）

某些安全策略或階段限制（依平台）

5.5 為什麼「顯示 256 但只能改 Bytes」是合理的

UI 想固定 16x16，不然 Bytes < 256 時畫面會不滿格

但你希望「使用者輸入多大就只能改多大」，避免誤改到你額外分配出來的 padding 區

6. Hex Editor 的內部邏輯（你之後會常用）
6.1 指標與索引關係

Buf：分配到的起始指標

BaseOffset：翻頁偏移（0, 256, 512…）

Cursor：頁內游標（0..255）

全域索引：GlobalIdx = BaseOffset + Cursor

實際要寫的位置：Buf[GlobalIdx]

6.2 「限制可寫範圍」的判斷（核心）

if (GlobalIdx >= EditSize) { 禁止修改 }

Pages 模式：EditSize = TotalBytes

Pools 模式：EditSize = Bytes（你輸入值）

6.3 常見擴充（建議你下一步加）

顯示 ASCII 欄（右側 16 字元）

顯示游標位址：Current = (UINTN)Buf + GlobalIdx

支援一次輸入 16 bytes（貼 hex string）

增加「搜尋 pattern」功能

7. 你這份工具的「開發/除錯重點筆記」
7.1 任何 Boot Services API 都看 EFI_STATUS

用 %r 印出錯誤最省時間
Print(L"xxx: %r\n", Status);

7.2 指標轉整數顯示

位址顯示常用：

%p（印指標）

%lx（印 UINT64 位址）

轉型要小心：

(UINTN)Ptr → 平台指標寬度

(UINT64)(UINTN)Ptr → 轉成 64-bit 印

7.3 Pool vs Pages 的差異

Pages：以 頁為單位，會回傳 EFI_PHYSICAL_ADDRESS

Pool：以 byte為單位，回傳 VOID*（虛擬/實體概念由實作決定，但在 Boot 時通常 identity mapping）


cd /d D:\BIOS\MyWorkSpace\edk2

edksetup.bat Rebuild

chcp 65001

set PYTHONUTF8=1

set PYTHONIOENCODING=utf-8

rmdir /s /q Build\MemoryUtilityPkg

build -p MemoryUtilityPkg\MemoryUtilityPkg.dsc -a X64 -t VS2019 -b DEBUG
