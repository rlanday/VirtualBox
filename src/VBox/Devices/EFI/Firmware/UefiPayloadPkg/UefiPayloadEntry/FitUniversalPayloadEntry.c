/** @file
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UefiPayloadEntry.h"
#include <Library/FdtLib.h>
#include <Guid/UniversalPayloadBase.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

extern VOID  *mHobList;

CHAR8  *mLineBuffer = NULL;

/**
  Print all HOBs info from the HOB list.
  @return The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  );

/**
  Find the first substring.
  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.
**/
UINTN
EFIAPI
AsciiStrSpn (
  IN CHAR8  *String,
  IN CHAR8  *CharSet
  )
{
  UINTN  Count;
  CHAR8  *Str1;
  CHAR8  *Str2;

  Count = 0;

  for (Str1 = String; *Str1 != L'\0'; Str1++) {
    for (Str2 = CharSet; *Str2 != L'\0'; Str2++) {
      if (*Str1 == *Str2) {
        break;
      }
    }

    if (*Str2 == L'\0') {
      return Count;
    }

    Count++;
  }

  return Count;
}

/**
  Searches a string for the first occurrence of a character contained in a
  specified buffer.
  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.
**/
CHAR8 *
EFIAPI
AsciiStrBrk (
  IN CHAR8  *String,
  IN CHAR8  *CharSet
  )
{
  CHAR8  *Str1;
  CHAR8  *Str2;

  for (Str1 = String; *Str1 != L'\0'; Str1++) {
    for (Str2 = CharSet; *Str2 != L'\0'; Str2++) {
      if (*Str1 == *Str2) {
        return (CHAR8 *)Str1;
      }
    }
  }

  return NULL;
}

/**
  Find the next token after one or more specified characters.
  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.
**/
CHAR8 *
EFIAPI
AsciiStrTokenLine (
  IN CHAR8  *String OPTIONAL,
  IN CHAR8  *CharSet
  )
{
  CHAR8  *Begin;
  CHAR8  *End;

  Begin = (String == NULL) ? mLineBuffer : String;
  if (Begin == NULL) {
    return NULL;
  }

  Begin += AsciiStrSpn (Begin, CharSet);
  if (*Begin == L'\0') {
    mLineBuffer = NULL;
    return NULL;
  }

  End = AsciiStrBrk (Begin, CharSet);
  if ((End != NULL) && (*End != L'\0')) {
    *End = L'\0';
    End++;
  }

  mLineBuffer = End;
  return Begin;
}

/**
  Some bootloader may pass a pcd database, and UPL also contain a PCD database.
  Dxe PCD driver has the assumption that the two PCD database can be catenated and
  the local token number should be successive.
  This function will fix up the UPL PCD database to meet that assumption.
  @param[in]   DxeFv         The FV where to find the Universal PCD database.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval other              Failed to fix up.
**/
EFI_STATUS
FixUpPcdDatabase (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv
  )
{
  EFI_STATUS           Status;
  EFI_FFS_FILE_HEADER  *FileHeader;
  VOID                 *PcdRawData;
  PEI_PCD_DATABASE     *PeiDatabase;
  PEI_PCD_DATABASE     *UplDatabase;
  EFI_HOB_GUID_TYPE    *GuidHob;
  DYNAMICEX_MAPPING    *ExMapTable;
  UINTN                Index;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  if (GuidHob == NULL) {
    //
    // No fix-up is needed.
    //
    return EFI_SUCCESS;
  }

  PeiDatabase = (PEI_PCD_DATABASE *)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "Find the Pei PCD data base, the total local token number is %d\n", PeiDatabase->LocalTokenCount));

  Status = FvFindFileByTypeGuid (DxeFv, EFI_FV_FILETYPE_DRIVER, PcdGetPtr (PcdPcdDriverFile), &FileHeader);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileFindSection (FileHeader, EFI_SECTION_RAW, &PcdRawData);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UplDatabase = (PEI_PCD_DATABASE *)PcdRawData;
  ExMapTable  = (DYNAMICEX_MAPPING *)(UINTN)((UINTN)PcdRawData + UplDatabase->ExMapTableOffset);

  for (Index = 0; Index < UplDatabase->ExTokenCount; Index++) {
    ExMapTable[Index].TokenNumber += PeiDatabase->LocalTokenCount;
  }

  DEBUG ((DEBUG_INFO, "Fix up UPL PCD database successfully\n"));
  return EFI_SUCCESS;
}

/**
  Add HOB into HOB list
  @param[in]  Hob    The HOB to be added into the HOB list.
**/
VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS  *Hob
  )
{
  EFI_PEI_HOB_POINTERS  NewHob;

  if (Hob->Raw == NULL) {
    return;
  }

  NewHob.Header = CreateHob (Hob->Header->HobType, Hob->Header->HobLength);
  ASSERT (NewHob.Header != NULL);
  if (NewHob.Header == NULL) {
    return;
  }

  CopyMem (NewHob.Header + 1, Hob->Header + 1, Hob->Header->HobLength - sizeof (EFI_HOB_GENERIC_HEADER));
}

/**
  Found the Resource Descriptor HOB that contains a range (Base, Top)
  @param[in] HobList    Hob start address
  @param[in] Base       Memory start address
  @param[in] Top        Memory end address.
  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindResourceDescriptorByRange (
  IN VOID                  *HobList,
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN EFI_PHYSICAL_ADDRESS  Top
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }

    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not contain the PHIT range EfiFreeMemoryBottom..EfiFreeMemoryTop
    //
    if (Base < ResourceHob->PhysicalStart) {
      continue;
    }

    if (Top > (ResourceHob->PhysicalStart + ResourceHob->ResourceLength)) {
      continue;
    }

    return ResourceHob;
  }

  return NULL;
}

/**
  Find the highest below 4G memory resource descriptor, except the input Resource Descriptor.
  @param[in] HobList                 Hob start address
  @param[in] MinimalNeededSize       Minimal needed size.
  @param[in] ExceptResourceHob       Ignore this Resource Descriptor.
  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindAnotherHighestBelow4GResourceDescriptor (
  IN VOID                         *HobList,
  IN UINTN                        MinimalNeededSize,
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ExceptResourceHob
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ReturnResourceHob;

  ReturnResourceHob = NULL;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }

    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip if the Resource Descriptor HOB equals to ExceptResourceHob
    //
    if (ResourceHob == ExceptResourceHob) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that are beyond 4G
    //
    if ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength) > BASE_4GB) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that are too small
    //
    if (ResourceHob->ResourceLength < MinimalNeededSize) {
      continue;
    }

    //
    // Return the topest Resource Descriptor
    //
    if (ReturnResourceHob == NULL) {
      ReturnResourceHob = ResourceHob;
    } else {
      if (ReturnResourceHob->PhysicalStart < ResourceHob->PhysicalStart) {
        ReturnResourceHob = ResourceHob;
      }
    }
  }

  return ReturnResourceHob;
}

/**
  Check the HOB and decide if it is need inside Payload
  Payload maintainer may make decision which HOB is need or needn't
  Then add the check logic in the function.
  @param[in] Hob The HOB to check
  @retval TRUE  If HOB is need inside Payload
  @retval FALSE If HOB is needn't inside Payload
**/
BOOLEAN
IsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  )
{
  if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
    return FALSE;
  }

  if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
    if (CompareGuid (&Hob.MemoryAllocationModule->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid)) {
      return FALSE;
    }
  }

  // Arrive here mean the HOB is need
  return TRUE;
}

/**
  It will build Fv HOBs based on information from bootloaders.
  @param[out] DxeFv          The pointer to the DXE FV in memory.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_NOT_FOUND      If it failed to find node in fit image.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildFitLoadablesFvHob (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_STATUS              Status;
  VOID                    *Fdt;
  UINT8                   *GuidHob;
  UNIVERSAL_PAYLOAD_BASE  *PayloadBase;
  INT32                   ConfigNode;
  INT32                   Config1Node;
  INT32                   ImageNode;
  INT32                   FvNode;
  INT32                   Depth;
  CONST FDT_PROPERTY      *PropertyPtr;
  INT32                   TempLen;
  CONST CHAR8             *Fvname;
  UINT32                  DataOffset;
  UINT32                  DataSize;
  UINT32                  *Data32;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadBaseGuid);
  if (GuidHob != NULL) {
    PayloadBase = (UNIVERSAL_PAYLOAD_BASE *)GET_GUID_HOB_DATA (GuidHob);
    Fdt         = (VOID *)(UINTN)PayloadBase->Entry;
    DEBUG ((DEBUG_INFO, "PayloadBase Entry = 0x%08x\n", PayloadBase->Entry));
  }

  Status = FdtCheckHeader (Fdt);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  ConfigNode = FdtSubnodeOffsetNameLen (Fdt, 0, "configurations", (INT32)AsciiStrLen ("configurations"));
  if (ConfigNode <= 0) {
    return EFI_NOT_FOUND;
  }

  Config1Node = FdtSubnodeOffsetNameLen (Fdt, ConfigNode, "conf-1", (INT32)AsciiStrLen ("conf-1"));
  if (Config1Node <= 0) {
    return EFI_NOT_FOUND;
  }

  ImageNode = FdtSubnodeOffsetNameLen (Fdt, 0, "images", (INT32)AsciiStrLen ("images"));
  if (ImageNode <= 0) {
    return EFI_NOT_FOUND;
  }

  FvNode = FdtSubnodeOffsetNameLen (Fdt, ImageNode, "tianocore", (INT32)AsciiStrLen ("tianocore"));
  Depth  = FdtNodeDepth (Fdt, FvNode);
  FvNode = FdtNextNode (Fdt, FvNode, &Depth);
  Fvname = FdtGetName (Fdt, FvNode, &TempLen);
  while ((AsciiStrCmp ((Fvname + AsciiStrLen (Fvname) - 2), "fv") == 0)) {
    if (FvNode <= 0) {
      return EFI_NOT_FOUND;
    }

    PropertyPtr = FdtGetProperty (Fdt, FvNode, "data-offset", &TempLen);
    Data32      = (UINT32 *)(PropertyPtr->Data);
    DataOffset  = SwapBytes32 (*Data32);

    PropertyPtr = FdtGetProperty (Fdt, FvNode, "data-size", &TempLen);
    Data32      = (UINT32 *)(PropertyPtr->Data);
    DataSize    = SwapBytes32 (*Data32);

    if (AsciiStrCmp (Fvname, "uefi-fv") == 0) {
      *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)PayloadBase->Entry + (UINTN)DataOffset);
      ASSERT ((*DxeFv)->FvLength == DataSize);
    } else {
      BuildFvHob (((UINTN)PayloadBase->Entry + (UINTN)DataOffset), DataSize);
    }

    DEBUG ((
      DEBUG_INFO,
      "UPL Multiple fv[%a], Base=0x%08x, size=0x%08x\n",
      Fvname,
      ((UINTN)PayloadBase->Entry + (UINTN)DataOffset),
      DataSize
      ));
    Depth  = FdtNodeDepth (Fdt, FvNode);
    FvNode = FdtNextNode (Fdt, FvNode, &Depth);
    Fvname = FdtGetName (Fdt, FvNode, &TempLen);
  }

  return EFI_SUCCESS;
}

/**
  It will build HOBs based on information from bootloaders.
  @param[in]  BootloaderParameter   The starting memory address of bootloader parameter block.
  @param[out] DxeFv                 The pointer to the DXE FV in memory.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobs (
  IN  UINTN                       BootloaderParameter,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  EFI_PEI_HOB_POINTERS          Hob;
  UINTN                         MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS          FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS          FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS          MemoryBottom;
  EFI_PHYSICAL_ADDRESS          MemoryTop;
  EFI_HOB_RESOURCE_DESCRIPTOR   *PhitResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR   *ResourceHob;
  UINT8                         *GuidHob;
  EFI_HOB_FIRMWARE_VOLUME       *FvHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTable;
  ACPI_BOARD_INFO               *AcpiBoardInfo;
  EFI_HOB_HANDOFF_INFO_TABLE    *HobInfo;

  Hob.Raw           = (UINT8 *)BootloaderParameter;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  ASSERT (Hob.Raw != NULL);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryTop == Hob.HandoffInformationTable->EfiFreeMemoryTop);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiMemoryTop == Hob.HandoffInformationTable->EfiMemoryTop);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiFreeMemoryBottom == Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  ASSERT ((UINTN)Hob.HandoffInformationTable->EfiMemoryBottom == Hob.HandoffInformationTable->EfiMemoryBottom);

  //
  // Try to find Resource Descriptor HOB that contains Hob range EfiMemoryBottom..EfiMemoryTop
  //
  PhitResourceHob = FindResourceDescriptorByRange (Hob.Raw, Hob.HandoffInformationTable->EfiMemoryBottom, Hob.HandoffInformationTable->EfiMemoryTop);
  if (PhitResourceHob == NULL) {
    //
    // Boot loader's Phit Hob is not in an available Resource Descriptor, find another Resource Descriptor for new Phit Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (Hob.Raw, MinimalNeededSize, NULL);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    MemoryTop        = FreeMemoryTop;
  } else if (PhitResourceHob->PhysicalStart + PhitResourceHob->ResourceLength - Hob.HandoffInformationTable->EfiMemoryTop >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right above memory top in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiFreeMemoryTop;
    FreeMemoryBottom = Hob.HandoffInformationTable->EfiMemoryTop;
    FreeMemoryTop    = FreeMemoryBottom + MinimalNeededSize;
    MemoryTop        = FreeMemoryTop;
  } else if (Hob.HandoffInformationTable->EfiMemoryBottom - PhitResourceHob->PhysicalStart >= MinimalNeededSize) {
    //
    // New availiable Memory range in new hob is right below memory bottom in old hob.
    //
    MemoryBottom     = Hob.HandoffInformationTable->EfiMemoryBottom - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = Hob.HandoffInformationTable->EfiMemoryBottom;
    MemoryTop        = Hob.HandoffInformationTable->EfiMemoryTop;
  } else {
    //
    // In the Resource Descriptor HOB contains boot loader Hob, there is no enough free memory size for payload hob
    // Find another Resource Descriptor Hob
    //
    ResourceHob = FindAnotherHighestBelow4GResourceDescriptor (Hob.Raw, MinimalNeededSize, PhitResourceHob);
    if (ResourceHob == NULL) {
      return EFI_NOT_FOUND;
    }

    MemoryBottom     = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MinimalNeededSize;
    FreeMemoryBottom = MemoryBottom;
    FreeMemoryTop    = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    MemoryTop        = FreeMemoryTop;
  }

  HobInfo           = HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
  HobInfo->BootMode = Hob.HandoffInformationTable->BootMode;
  //
  // From now on, mHobList will point to the new Hob range.
  //

  //
  // Create an empty FvHob for the DXE FV that contains DXE core.
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS)0, 0);
  //
  // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (IsHobNeed (Hob)) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  BuildFitLoadablesFvHob (DxeFv);

  //
  // Create guid hob for acpi board information
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
  if (GuidHob != NULL) {
    AcpiTable = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);
    GuidHob   = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
    if (GuidHob == NULL) {
      AcpiBoardInfo = BuildHobFromAcpi ((UINT64)AcpiTable->Rsdp);
      ASSERT (AcpiBoardInfo != NULL);
    }
  }

  //
  // Update DXE FV information to first fv hob in the hob list, which
  // is the empty FvHob created before.
  //
  FvHob              = GetFirstHob (EFI_HOB_TYPE_FV);
  FvHob->BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)*DxeFv;
  FvHob->Length      = (*DxeFv)->FvLength;
  return EFI_SUCCESS;
}

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS                  Status;
  PHYSICAL_ADDRESS            DxeCoreEntryPoint;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_FIRMWARE_VOLUME_HEADER  *DxeFv;

  mHobList = (VOID *)BootloaderParameter;
  DxeFv    = NULL;
  // Call constructor for all libraries
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "Entering Universal Payload...\n"));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));

  DEBUG_CODE (
    //
    // Dump the Hobs from boot loader
    //
    PrintHob (mHobList);
    );

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // Build HOB based on information from Bootloader
  Status = BuildHobs (BootloaderParameter, &DxeFv);
  ASSERT_EFI_ERROR (Status);

  FixUpPcdDatabase (DxeFv);
  Status = UniversalLoadDxeCore (DxeFv, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0xFF);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *)GetFirstHob (EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
