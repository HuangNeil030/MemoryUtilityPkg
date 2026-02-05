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

1) 整體主流程圖（Main Menu）
┌──────────────────────────────┐
│            UefiMain           │
└───────────────┬──────────────┘
                │
                v
      ┌────────────────────┐
      │ SetNormalAttr()     │
      │ Page_Main()         │
      └─────────┬──────────┘
                │
                v
   ┌──────────────────────────────┐
   │ MenuSelect "<<Select action>>"│
   │  0) Dump Memory Map           │
   │  1) Allocate Memory           │
   │  ESC -> return (exit)         │
   └─────────┬───────────┬────────┘
             │           │
             v           v
   ┌────────────────┐  ┌───────────────────┐
   │ Page_Dump...() │  │ Page_Allocate...() │
   └───────┬────────┘  └─────────┬─────────┘
           │                     │
           v                     v
   (任意鍵返回主選單)      (返回主選單直到 ESC)

2) Dump Memory Map 流程圖
┌──────────────────────────────┐
│       Page_DumpMemoryMap      │
└───────────────┬──────────────┘
                │
                v
   ┌──────────────────────────────┐
   │ GetMemoryMap(MapSize=0,Map=0) │
   └───────────────┬──────────────┘
                   │
     ┌─────────────┴─────────────┐
     │ Status != EFI_BUFFER_TOO_SMALL? │
     └───────┬───────────┬────────┘
             │Yes        │No(正常)
             v           v
   ┌─────────────────┐  ┌─────────────────────────┐
   │ 顯示錯誤 + 等按鍵 │  │ MapSize += DescSize*16  │
   │ return           │  └───────────┬─────────────┘
   └─────────────────┘              │
                                    v
                     ┌───────────────────────────┐
                     │ AllocatePool(MapSize,&Map) │
                     └───────────┬───────────────┘
                                 │
                   ┌─────────────┴─────────────┐
                   │ EFI_ERROR(Status)?         │
                   └───────┬───────────┬────────┘
                           │Yes        │No
                           v           v
               ┌─────────────────┐  ┌───────────────────────────┐
               │ 顯示錯誤 + return │  │ GetMemoryMap(真正取得Map) │
               └─────────────────┘  └───────────┬───────────────┘
                                                │
                                  ┌─────────────┴─────────────┐
                                  │ EFI_ERROR(Status)?         │
                                  └───────┬───────────┬────────┘
                                          │Yes        │No
                                          v           v
                              ┌─────────────────┐  ┌───────────────────────────┐
                              │ 顯示錯誤+FreePool│  │ 逐條列印 Descriptor         │
                              │ + return         │  │ 每20行 Pause一次            │
                              └─────────────────┘  └───────────┬───────────────┘
                                                                │
                                                                v
                                                  ┌───────────────────────────┐
                                                  │ 統計 TypePages / Total MB  │
                                                  └───────────┬───────────────┘
                                                              │
                                                              v
                                                  ┌───────────────────────────┐
                                                  │ FreePool(Map) + 等按鍵返回 │
                                                  └───────────────────────────┘

3) Allocate Memory 子流程（Pages / Pools 選擇）

┌──────────────────────────────┐
│        Page_AllocateMemory    │
└───────────────┬──────────────┘
                │
                v
   ┌────────────────────────────────┐
   │ MenuSelect "<<Select allocate>>"│
   │ 0) Allocate Pages              │
   │ 1) Allocate Pools              │
   │ ESC -> return                  │
   └─────────────┬─────────┬────────┘
                 │         │
                 v         v
     ┌────────────────┐  ┌────────────────┐
     │ AllocatePages  │  │ AllocatePool   │
     │ Flow()         │  │ Flow()         │
     └────────────────┘  └────────────────┘

4) Allocate Pages Flow 流程圖

┌──────────────────────────────┐
│       AllocatePagesFlow       │
└───────────────┬──────────────┘
                │
                v
   ┌──────────────────────────────┐
   │ SelectMemoryType()            │
   │ ESC -> return                 │
   └───────────────┬──────────────┘
                   │
                   v
   ┌──────────────────────────────┐
   │ SelectAllocateType()          │
   │ (Any/MaxAddress/Address)      │
   │ ESC -> return                 │
   └───────────────┬──────────────┘
                   │
                   v
   ┌──────────────────────────────┐
   │ Prompt pages count            │
   │ (decimal or 0xHEX)            │
   └───────────────┬──────────────┘
                   │
        ┌──────────┴──────────┐
        │ AType需要地址?       │
        │ MaxAddress/Address   │
        └───────┬───────┬──────┘
                │Yes    │No
                v       v
   ┌──────────────────┐   (Addr=0)
   │ Prompt Addr (hex)│
   └─────────┬────────┘
             │
             v
   ┌─────────────────────────────────────────┐
   │ AllocatePages(AType, MType, Pages, &Addr)│
   └───────────────┬─────────────────────────┘
                   │
         ┌─────────┴─────────┐
         │ EFI_ERROR(Status)? │
         └───────┬───────┬───┘
                 │Yes    │No
                 v       v
   ┌──────────────────────┐   ┌──────────────────────────────┐
   │ Print failed + return │   │ Print Start~End              │
   └──────────────────────┘   └───────────────┬──────────────┘
                                               │
                                               v
                                ┌──────────────────────────────┐
                                │ Prompt "Edit Memory (y/n)"    │
                                └───────────┬───────────┬──────┘
                                            │Yes        │No
                                            v           v
                          ┌──────────────────────────┐   (skip)
                          │ Page_HexEdit(Addr, Size) │
                          └───────────┬──────────────┘
                                      │
                                      v
                                ┌──────────────────────────────┐
                                │ Prompt "Free Memory (y/n)"    │
                                └───────────┬───────────┬──────┘
                                            │Yes        │No
                                            v           v
                          ┌──────────────────────────────┐    (return)
                          │ FreePages(Addr, Pages)        │
                          │ success -> wait ESC return     │
                          └──────────────────────────────┘

5) Allocate Pools Flow（你要的：顯示全 256，但只允許改輸入範圍）

你需求：

畫面顯示：至少 0x100 (256 bytes)
可修改：只允許 0 .. (UserInputBytes-1)
Address range：要「印出完整顯示範圍」或「印出實際可寫範圍」都可以，但你說希望 不要只印輸入數量 → 我建議 兩段都印（最清楚）

┌──────────────────────────────┐
│       AllocatePoolFlow        │
└───────────────┬──────────────┘
                │
                v
   ┌──────────────────────────────┐
   │ SelectMemoryType()            │
   │ ESC -> return                 │
   └───────────────┬──────────────┘
                   │
                   v
   ┌──────────────────────────────┐
   │ Prompt Bytes (user request)   │
   └───────────────┬──────────────┘
                   │
                   v
   ┌───────────────────────────────────────┐
   │ AllocSize = max(Bytes, 256)           │
   │ AllocatePool(MType, AllocSize, &Buf)  │
   └───────────────┬───────────────────────┘
                   │
         ┌─────────┴─────────┐
         │ EFI_ERROR(Status)? │
         └───────┬───────┬───┘
                 │Yes    │No
                 v       v
   ┌──────────────────────┐   ┌──────────────────────────────┐
   │ Print failed + return │   │ Print address ranges          │
   └──────────────────────┘   │ - DisplayRange: Buf..Buf+AllocSize-1 │
                              │ - EditableRange: Buf..Buf+Bytes-1    │
                              └───────────────┬──────────────┘
                                              │
                                              v
                               ┌──────────────────────────────┐
                               │ Prompt "Edit Memory (y/n)"    │
                               └───────────┬───────────┬──────┘
                                           │Yes        │No
                                           v           v
                 ┌──────────────────────────────────────────────┐
                 │ Page_HexEdit(Buf, AllocSize, EditLimit=Bytes)│
                 │ Enter寫入前判斷: if (GlobalIdx>=Bytes) 禁止寫 │
                 └───────────┬──────────────────────────────────┘
                             │
                             v
                               ┌──────────────────────────────┐
                               │ Prompt "Free Memory (y/n)"    │
                               └───────────┬───────────┬──────┘
                                           │Yes        │No
                                           v           v
                           ┌──────────────────────────────┐   (return)
                           │ FreePool(Buf)                 │
                           │ success -> wait ESC return     │
                           └──────────────────────────────┘

6) Hex Editor（Page_HexEdit）流程圖

┌──────────────────────────────┐
│          Page_HexEdit         │
│ Buf, BufSize                  │
└───────────────┬──────────────┘
                │
         Cursor=0, BaseOffset=0
                │
                v
     ┌───────────────────────────┐
     │ ViewBase = Buf+BaseOffset  │
     │ ViewSize=min(BufSize-BaseOffset,256)│
     │ DrawHexEditor(ViewBase,ViewSize,Cursor)│
     └───────────────┬───────────┘
                     │
                     v
          ┌────────────────────┐
          │ Wait KeyStroke      │
          └─────────┬──────────┘
                    │
        ┌───────────┴─────────────────────────────────────┐
        │ 判斷按鍵                                          │
        └───────────┬─────────────────────────────────────┘
                    │
   ┌────────────────┼─────────────────────────────────────────────────────────┐
   │ ESC            │ 方向鍵                                                   │ PgUp/PgDn
   │ -> return      │ Cursor +/- 1 / +/-16 (邊界保護)                           │ BaseOffset +/-256 (邊界保護)
   └────────────────┼─────────────────────────────────────────────────────────┘
                    │
                    v
             Enter (edit)
                    │
                    v
        ┌──────────────────────────────┐
        │ ReadHexByte() 取得 NewVal     │
        └───────────────┬──────────────┘
                        │
          ┌─────────────┴─────────────┐
          │ (可選) 是否超出可寫範圍?   │  <-- Pools 用 Bytes 限制
          └───────┬───────────┬───────┘
                  │Yes        │No
                  v           v
       ┌────────────────┐   ┌──────────────────────┐
       │ 提示不可寫      │   │ ViewBase[Cursor]=NewVal│
       │ 等按鍵          │   └──────────────────────┘
       └────────────────┘


cd /d D:\BIOS\MyWorkSpace\edk2
edksetup.bat Rebuild
chcp 65001
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
rmdir /s /q Build\MemoryUtilityPkg
build -p MemoryUtilityPkg\MemoryUtilityPkg.dsc -a X64 -t VS2019 -b DEBUG
