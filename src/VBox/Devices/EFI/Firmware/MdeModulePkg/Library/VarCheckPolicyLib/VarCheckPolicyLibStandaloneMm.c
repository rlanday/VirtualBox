/** @file -- VarCheckPolicyLibStandaloneMm.c
This is an instance of a VarCheck lib constructor for Standalone MM.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/StandaloneMmMemLib.h>

#include "VarCheckPolicyLib.h"

/**
  Standalone MM constructor function of VarCheckPolicyLib to invoke common
  constructor routine.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPolicyLibStandaloneConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return VarCheckPolicyLibCommonConstructor ();
}

/**
  This function is wrapper function to validate the Primary Buffer (CommBuffer).

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid.
  @retval FALSE This buffer is not valid.
**/
BOOLEAN
EFIAPI
VarCheckPolicyIsPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return TRUE;
}
