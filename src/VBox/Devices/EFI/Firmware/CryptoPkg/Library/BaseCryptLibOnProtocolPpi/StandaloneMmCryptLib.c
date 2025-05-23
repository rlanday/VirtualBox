/** @file
  Implements the GetCryptoServices() API that retuns a pointer to the EDK II
  SMM Crypto Protocol.

  Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/SmmCrypto.h>

EDKII_SMM_CRYPTO_PROTOCOL  *mSmmCryptoProtocol = NULL;

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI.  The layout of the PPI, DXE Protocol, and SMM Protocol are
  identical which allows the implementation of the BaseCryptLib functions that
  call through a Protocol/PPI to be shared for the PEI, DXE, and SMM
  implementations.

  This SMM implementation returns the pointer to the EDK II SMM Crypto Protocol
  that was found in the library constructor SmmCryptLibConstructor().
**/
VOID *
GetCryptoServices (
  VOID
  )
{
  return (VOID *)mSmmCryptoProtocol;
}

/**
  Constructor looks up the EDK II SMM Crypto Protocol and verifies that it is
  not NULL and has a high enough version value to support all the BaseCryptLib
  functions.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  MmSystemTable A pointer to the MM System Table.

  @retval  EFI_SUCCESS    The EDK II SMM Crypto Protocol was found.
  @retval  EFI_NOT_FOUND  The EDK II SMM Crypto Protocol was not found.
**/
EFI_STATUS
EFIAPI
StandaloneMmCryptLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Version;

  Status = gMmst->MmLocateProtocol (
                    &gEdkiiSmmCryptoProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCryptoProtocol
                    );
  if (EFI_ERROR (Status) || (mSmmCryptoProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "[StandaloneMmCryptLib] Failed to locate Crypto SMM Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (mSmmCryptoProtocol != NULL);
    mSmmCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  Version = mSmmCryptoProtocol->GetVersion ();
  if (Version < EDKII_CRYPTO_VERSION) {
    DEBUG ((DEBUG_ERROR, "[StandaloneMmCryptLib] Crypto SMM Protocol unsupported version %d\n", Version));
    ASSERT (Version >= EDKII_CRYPTO_VERSION);
    mSmmCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
