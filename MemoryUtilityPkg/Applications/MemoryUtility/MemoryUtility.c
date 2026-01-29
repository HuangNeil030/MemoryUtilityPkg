#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextOut.h>

//
// 定義顏色與常數
//
#define COLOR_NORMAL      EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)
#define COLOR_HIGHLIGHT   EFI_TEXT_ATTR(EFI_WHITE, EFI_BLUE)
#define COLOR_HEADER      EFI_TEXT_ATTR(EFI_WHITE, EFI_BLUE)
#define COLOR_OFFSET      EFI_TEXT_ATTR(EFI_MAGENTA, EFI_BLACK)
#define COLOR_EDIT_CURSOR EFI_TEXT_ATTR(EFI_WHITE, EFI_LIGHTBLUE)
#define MAX_STRING_LEN    64

//
// 記憶體類型字串陣列
//
CHAR16 *mMemoryTypeStrings[] = {
  L"EfiReservedMemoryType",
  L"EfiLoaderCode",
  L"EfiLoaderData",
  L"EfiBootServicesCode",
  L"EfiBootServicesData",
  L"EfiRuntimeServicesCode",
  L"EfiRuntimeServicesData",
  L"EfiConventionalMemory",
  L"EfiUnusableMemory",
  L"EfiACPIReclaimMemory",
  L"EfiACPIMemoryNVS",
  L"EfiMemoryMappedIO",
  L"EfiMemoryMappedIOPortSpace",
  L"EfiPalCode",
  L"EfiPersistentMemory",
  L"EfiMaxMemoryType"
};

//
// 分配類型字串陣列
//
CHAR16 *mAllocateTypeStrings[] = {
  L"AllocateAnyPages",
  L"AllocateMaxAddress",
  L"AllocateAddress",
  L"MaxAllocateType"
};

// ==========================================
//  輔助函數 (UI Helpers)
// ==========================================

VOID
SetColor (
  UINTN Attribute
  )
{
  gST->ConOut->SetAttribute (gST->ConOut, Attribute);
}

VOID
ClearScreen (
  VOID
  )
{
  SetColor (COLOR_NORMAL);
  gST->ConOut->ClearScreen (gST->ConOut);
}

EFI_INPUT_KEY
WaitForKey (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  UINTN Index;

  Status = gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  return Key;
}

UINTN
GetInputNumber (
  IN CHAR16 *Prompt
  )
{
  EFI_INPUT_KEY Key;
  CHAR16 Buffer[MAX_STRING_LEN];
  UINTN Count;
  UINTN Num;

  Count = 0;
  Num = 0;

  Print (L"%s", Prompt);

  while (TRUE) {
    Key = WaitForKey ();

    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      Print (L"\n");
      break;
    } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
      if (Count > 0) {
        Count--;
        Print (L"\b \b");
      }
    } else if (Key.UnicodeChar >= L'0' && Key.UnicodeChar <= L'9') {
      if (Count < MAX_STRING_LEN - 1) {
        Buffer[Count] = Key.UnicodeChar;
        Print (L"%c", Key.UnicodeChar);
        Count++;
      }
    }
  }

  Buffer[Count] = L'\0';
  Num = StrDecimalToUintn (Buffer);
  return Num;
}

UINTN
DrawMenu (
  IN CHAR16 *Title,
  IN CHAR16 **Items,
  IN UINTN  ItemCount
  )
{
  UINTN SelectedIndex;
  EFI_INPUT_KEY Key;
  UINTN i;

  SelectedIndex = 0;

  while (TRUE) {
    ClearScreen ();
    
    SetColor (COLOR_HEADER);
    Print (L"<<%s>>\n", Title);
    SetColor (COLOR_NORMAL);

    for (i = 0; i < ItemCount; i++) {
      if (i == SelectedIndex) {
        SetColor (COLOR_HIGHLIGHT);
      } else {
        SetColor (COLOR_NORMAL);
      }
      Print (L"%s\n", Items[i]);
    }
    SetColor (COLOR_NORMAL);

    Key = WaitForKey ();

    if (Key.ScanCode == SCAN_UP) {
      if (SelectedIndex > 0) SelectedIndex--;
    } else if (Key.ScanCode == SCAN_DOWN) {
      if (SelectedIndex < ItemCount - 1) SelectedIndex++;
    } else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      return SelectedIndex;
    } else if (Key.ScanCode == SCAN_ESC) {
      return (UINTN)-1;
    }
  }
}

// ==========================================
//  功能實作 (Business Logic)
// ==========================================

VOID
EditMemory (
  IN EFI_PHYSICAL_ADDRESS StartAddress,
  IN UINTN                Size
  )
{
  EFI_INPUT_KEY Key;
  UINTN         BaseOffset;
  UINTN         CursorX;
  UINTN         CursorY;
  UINT8         *MemPtr;
  BOOLEAN       Redraw;
  UINTN         RowsPerPage;
  UINTN         i, j;
  UINTN         CurrentAbsOffset;
  UINTN         DisplayLimit;
  UINT8         InputVal;
  UINT8         OldVal;

  BaseOffset = 0;
  CursorX = 0;
  CursorY = 0;
  MemPtr = (UINT8 *)(UINTN)StartAddress;
  Redraw = TRUE;
  RowsPerPage = 16;
  
  // 設定顯示極限：若分配 Size 小於 256 (一頁)，強制設定為 256
  DisplayLimit = (Size < 256) ? 256 : Size;

  while (TRUE) {
    if (Redraw) {
      ClearScreen ();

      SetColor (COLOR_HEADER);
      Print (L"MemoryAddress : %08lX               \n", StartAddress + BaseOffset);
      
      gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
      Print (L"  %08lX\n", StartAddress + BaseOffset);

      SetColor (COLOR_OFFSET);
      Print (L"     ");
      for (i = 0; i < 16; i++) {
        Print (L"%02X ", i);
      }
      Print (L"\n");

      for (i = 0; i < RowsPerPage; i++) {
        SetColor (COLOR_OFFSET);
        Print (L"%02X : ", (i * 16));

        for (j = 0; j < 16; j++) {
          CurrentAbsOffset = BaseOffset + (i * 16) + j;
          
          if (i == CursorY && j == CursorX) {
            SetColor (COLOR_HIGHLIGHT);
          } else {
            SetColor (COLOR_NORMAL);
          }
          Print (L"%02X ", MemPtr[CurrentAbsOffset]);
        }
        
        SetColor (COLOR_NORMAL);
        if (i == 3) Print (L"  Press Enter edit config");
        if (i == 8) Print (L"  Press PgUp or PgDn");
        if (i == 9) Print (L"  switch address display");

        Print (L"\n");
      }

      SetColor (COLOR_HEADER);
      Print (L"Press ESC Exit edit                    \n");
      SetColor (COLOR_NORMAL);
      
      Redraw = FALSE;
    }

    Key = WaitForKey ();

    if (Key.ScanCode == SCAN_ESC) {
      break;
    } else if (Key.ScanCode == SCAN_UP) {
      if (CursorY > 0) {
        CursorY--;
        Redraw = TRUE;
      }
    } else if (Key.ScanCode == SCAN_DOWN) {
      if (CursorY < RowsPerPage - 1) {
        CursorY++;
        Redraw = TRUE;
      }
    } else if (Key.ScanCode == SCAN_LEFT) {
      if (CursorX > 0) {
        CursorX--;
        Redraw = TRUE;
      }
    } else if (Key.ScanCode == SCAN_RIGHT) {
      if (CursorX < 15) {
        CursorX++;
        Redraw = TRUE;
      }
    } else if (Key.ScanCode == SCAN_PAGE_UP) {
      if (BaseOffset >= 0x100) {
        BaseOffset -= 0x100;
        Redraw = TRUE;
      }
    } else if (Key.ScanCode == SCAN_PAGE_DOWN) {
      if ((BaseOffset + 0x100) < DisplayLimit) {
        BaseOffset += 0x100;
        Redraw = TRUE;
      }
    } else {
      InputVal = 0xFF;
      
      if (Key.UnicodeChar >= L'0' && Key.UnicodeChar <= L'9') {
        InputVal = (UINT8)(Key.UnicodeChar - L'0');
      } else if (Key.UnicodeChar >= L'a' && Key.UnicodeChar <= L'f') {
        InputVal = (UINT8)(Key.UnicodeChar - L'a' + 10);
      } else if (Key.UnicodeChar >= L'A' && Key.UnicodeChar <= L'F') {
        InputVal = (UINT8)(Key.UnicodeChar - L'A' + 10);
      }

      if (InputVal != 0xFF) {
        CurrentAbsOffset = BaseOffset + (CursorY * 16) + CursorX;
        OldVal = MemPtr[CurrentAbsOffset];
        MemPtr[CurrentAbsOffset] = (OldVal << 4) | InputVal;
        Redraw = TRUE;
      }
    }
  }
}

VOID
DoDumpMemoryMap (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN MapKey, DescriptorSize;
  UINT32 DescriptorVersion;
  UINTN i, Count;
  EFI_MEMORY_DESCRIPTOR *Desc;
  UINT64 TotalPages;
  UINT64 TypePageCounts[16]; 
  CHAR16 *TypeStr;
  CHAR16 *ShortName;

  MemoryMapSize = 0;
  MemoryMap = NULL;
  TotalPages = 0;
  SetMem (TypePageCounts, sizeof (TypePageCounts), 0);

  Status = gBS->GetMemoryMap (&MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    MemoryMapSize += DescriptorSize * 16;
    Status = gBS->AllocatePool (EfiLoaderData, MemoryMapSize, (VOID**)&MemoryMap);
  }

  if (EFI_ERROR (Status)) {
    Print (L"GetMemoryMap Error!\n");
    WaitForKey ();
    return;
  }

  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR (Status)) {
    Print (L"GetMemoryMap Error 2!\n");
    gBS->FreePool (MemoryMap);
    WaitForKey ();
    return;
  }

  ClearScreen ();
  Print (L"Type                      Start              End                Pages\n");
  Print (L"----------------------------------------------------------------------\n");

  Count = MemoryMapSize / DescriptorSize;
  for (i = 0; i < Count; i++) {
    Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + (i * DescriptorSize));
    
    TotalPages += Desc->NumberOfPages;

    if (Desc->Type < 16) {
      TypePageCounts[Desc->Type] += Desc->NumberOfPages;
    }

    TypeStr = (Desc->Type < 16) ? mMemoryTypeStrings[Desc->Type] : L"Unknown";

    Print (L"%-25s %016lx-%016lx %08lx\n", 
      TypeStr,
      Desc->PhysicalStart,
      Desc->PhysicalStart + (Desc->NumberOfPages * 4096) - 1,
      Desc->NumberOfPages
    );

    if ((i + 1) % 18 == 0) {
      Print (L"-- Press any key --");
      WaitForKey ();
      ClearScreen ();
      Print (L"Type                      Start              End                Pages\n");
    }
  }

  gBS->FreePool (MemoryMap);

  Print (L"----------------------------------------------------------------------\n");
  
  for (i = 0; i < 16; i++) {
    if (TypePageCounts[i] > 0) {
      ShortName = mMemoryTypeStrings[i];
      if (StrnCmp(ShortName, L"Efi", 3) == 0) {
          ShortName += 3; 
      }
      Print (L"%-25s : %10ld Pages\n", ShortName, TypePageCounts[i]);
    }
  }

  Print (L"----------------------------------------------------------------------\n");
  Print (L"Total Memory:       %ld MB\n", TotalPages / 256);
  Print (L"\nfs4:\\> "); 
  WaitForKey ();
}

VOID
DoAllocateMemory (
  VOID
  )
{
  CHAR16 *SizeOpts[] = { L"Allocate Pages", L"Allocate Pools" };
  UINTN SizeSel;
  BOOLEAN IsPage;
  UINTN TypeSel;
  UINTN AllocTypeSel;
  UINTN Amount;
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS AllocatedAddr;
  UINTN AllocatedSize;
  VOID *Buffer;
  EFI_INPUT_KEY Key;
  BOOLEAN DidEdit;
  EFI_STATUS FreeStatus;

  AllocatedAddr = 0;
  AllocatedSize = 0;
  AllocTypeSel = 0;

  SizeSel = DrawMenu (L"Select the allocate size", SizeOpts, 2);
  if (SizeSel == (UINTN)-1) return;
  IsPage = (SizeSel == 0);

  TypeSel = DrawMenu (L"Select The Memory Type", mMemoryTypeStrings, 16);
  if (TypeSel == (UINTN)-1) return;

  if (IsPage) {
    AllocTypeSel = DrawMenu (L"Select The Allocate Type", mAllocateTypeStrings, 4);
    if (AllocTypeSel == (UINTN)-1) return;
  }

  ClearScreen ();
  SetColor (COLOR_HEADER);
  Print (L"<<Select The Allocate Type>>\n");
  SetColor (COLOR_NORMAL);
  Print (L"%s\n", IsPage ? mAllocateTypeStrings[AllocTypeSel] : L"N/A for Pool");
  
  Amount = GetInputNumber (L"How many memory want to allocate : ");
  if (Amount == 0) return;

  if (IsPage) {
    Status = gBS->AllocatePages ((EFI_ALLOCATE_TYPE)AllocTypeSel, (EFI_MEMORY_TYPE)TypeSel, Amount, &AllocatedAddr);
    AllocatedSize = Amount * 4096;
  } else {
    Status = gBS->AllocatePool ((EFI_MEMORY_TYPE)TypeSel, Amount, &Buffer);
    AllocatedAddr = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
    AllocatedSize = Amount;
  }

  ClearScreen ();
  if (EFI_ERROR (Status)) {
    Print (L"Allocate Failed: %r\n", Status);
    WaitForKey ();
    return;
  }

  while (TRUE) {
    ClearScreen();
    Print (L"Allocate Memory Success !!!\n\n");
    Print (L"The Memory Address is %08lX - %08lX\n\n", AllocatedAddr, AllocatedAddr + AllocatedSize - 1);
    
    // 1. Edit
    Print (L"Edit Memory Data (y/n) ");
    DidEdit = FALSE;
    while (TRUE) {
      Key = WaitForKey ();
      if (Key.UnicodeChar == L'y' || Key.UnicodeChar == L'Y') {
        Print(L"y\n");
        EditMemory (AllocatedAddr, AllocatedSize);
        DidEdit = TRUE;
        break;
      } else if (Key.UnicodeChar == L'n' || Key.UnicodeChar == L'N') {
        Print(L"n\n");
        break;
      }
    }

    if (DidEdit) {
      continue;
    }

    // 2. Free
    Print (L"Free Memory Data (y/n) ");
    while (TRUE) {
      Key = WaitForKey ();
      if (Key.UnicodeChar == L'y' || Key.UnicodeChar == L'Y') {
          Print(L"y\n");
          
          if (IsPage) {
              FreeStatus = gBS->FreePages(AllocatedAddr, Amount);
          } else {
              FreeStatus = gBS->FreePool((VOID*)(UINTN)AllocatedAddr);
          }

          if (!EFI_ERROR(FreeStatus)) {
              Print(L"\nFree Success, Press ESC to return");
          } else {
              Print(L"\nFree Failed: %r, Press ESC to return", FreeStatus);
          }

          while (TRUE) {
              Key = WaitForKey();
              if (Key.ScanCode == SCAN_ESC) {
                  return;
              }
          }
      } else if (Key.UnicodeChar == L'n' || Key.UnicodeChar == L'N') {
          Print(L"n\n");
          return;
      }
    }
  }
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  CHAR16 *MainOpts[] = { L"Dump Memory Map", L"Allocate Memory" };
  UINTN Sel;

  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  while (TRUE) {
    Sel = DrawMenu (L"Select the action", MainOpts, 2);
    
    if (Sel == 0) {
      DoDumpMemoryMap ();
    } else if (Sel == 1) {
      DoAllocateMemory ();
    } else {
      break;
    }
  }

  ClearScreen ();
  return EFI_SUCCESS;
}