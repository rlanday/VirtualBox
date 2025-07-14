/* $Id$ */
/** @file
 * GITS - Generic Interrupt Controller Interrupt Translation Service - Internal.
 */

/*
 * Copyright (C) 2025 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VMM_INCLUDED_SRC_include_GITSInternal_h
#define VMM_INCLUDED_SRC_include_GITSInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <VBox/types.h>
#include <VBox/gic-its.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/stam.h>

/** @defgroup grp_gits_int       Internal
 * @ingroup grp_gits
 * @internal
 * @{
 */

/** @name GITS Device Table Entry (DTE).
 * This gets stored to and loaded from guest memory.
 *  @{ */
#define GITS_BF_DTE_ITT_RANGE_SHIFT                 0
#define GITS_BF_DTE_ITT_RANGE_MASK                  UINT64_C(0x000000000000001f)
#define GITS_BF_DTE_RSVD_11_5_SHIFT                 5
#define GITS_BF_DTE_RSVD_11_5_MASK                  UINT64_C(0x0000000000000fe0)
#define GITS_BF_DTE_ITT_ADDR_SHIFT                  12
#define GITS_BF_DTE_ITT_ADDR_MASK                   UINT64_C(0x000ffffffffff000)
#define GITS_BF_DTE_RSVD_62_52_SHIFT                52
#define GITS_BF_DTE_RSVD_62_52_MASK                 UINT64_C(0x7ff0000000000000)
#define GITS_BF_DTE_VALID_SHIFT                     63
#define GITS_BF_DTE_VALID_MASK                      UINT64_C(0x8000000000000000)
RT_BF_ASSERT_COMPILE_CHECKS(GITS_BF_DTE_, UINT64_C(0), UINT64_MAX,
                            (ITT_RANGE, RSVD_11_5, ITT_ADDR, RSVD_62_52, VALID));
#define GITS_DTE_VALID_MASK                         (UINT64_MAX & ~(GITS_BF_DTE_RSVD_11_4_MASK | GITS_BF_DTE_RSVD_62_52_MASK));

/** GITS DTE. */
typedef uint64_t GITSDTE;
/** @} */

/** @name GITS Interrupt Translation Entry (ITE).
 * This gets stored to and loaded from guest memory.
 *
 * We use the full 64-bit format despite currently not supporting virtual INTIDs as
 * in the future accomodating changes to size/layout of data that resides in guest
 * memory is tedious.
 * @{ */
#define GITS_BF_ITE_VPEID_SHIFT                     0
#define GITS_BF_ITE_VPEID_MASK                      UINT64_C(0x000000000000ffff)
#define GITS_BF_ITE_ICID_SHIFT                      16
#define GITS_BF_ITE_ICID_MASK                       UINT64_C(0x00000000ffff0000)
#define GITS_BF_ITE_HYPER_INTID_SHIFT               32
#define GITS_BF_ITE_HYPER_INTID_MASK                UINT64_C(0x00007fff00000000)
#define GITS_BF_ITE_INTID_SHIFT                     47
#define GITS_BF_ITE_INTID_MASK                      UINT64_C(0x3fff800000000000)
#define GITS_BF_ITE_IS_PHYS_SHIFT                   62
#define GITS_BF_ITE_IS_PHYS_MASK                    UINT64_C(0x4000000000000000)
#define GITS_BF_ITE_VALID_SHIFT                     63
#define GITS_BF_ITE_VALID_MASK                      UINT64_C(0x8000000000000000)
RT_BF_ASSERT_COMPILE_CHECKS(GITS_BF_ITE_, UINT64_C(0), UINT64_MAX,
                            (VPEID, ICID, HYPER_INTID, INTID, IS_PHYS, VALID));

/** GITS ITE. */
typedef uint64_t GITSITE;
/** @} */

#if 0
/** @name GITS Collection Table Entry (CTE).
 * @{ */
#define GITS_BF_CTE_RDBASE_SHIFT                    0
#define GITS_BF_CTE_RDBASE_MASK                     UINT32_C(0x0000ffff)
#define GITS_BF_CTE_RSVD_30_16_SHIFT                16
#define GITS_BF_CTE_RSVD_30_16_MASK                 UINT32_C(0x7fff0000)
#define GITS_BF_CTE_VALID_SHIFT                     31
#define GITS_BF_CTE_VALID_MASK                      UINT32_C(0x80000000)
RT_BF_ASSERT_COMPILE_CHECKS(GITS_BF_CTE_, UINT32_C(0), UINT32_MAX,
                            (RDBASE, RSVD_30_16, VALID));
/** GITS CTE: Size of the CTE in bytes. */
#define GITS_CTE_SIZE                               4
/** @} */
#endif

/**
 * GITS error diagnostics.
 * Sorted alphabetically so it's easier to add and locate items, no other reason.
 *
 * @note Members of this enum are used as array indices, so no gaps in enum values
 *       are not allowed. Update @c g_apszGitsDiagDesc when you modify fields in
 *       this enum.
 */
typedef enum GITSDIAG
{
    /* No error, this must be zero! */
    kGitsDiag_None = 0,

    /* Command queue: basic operation errors. */
    kGitsDiag_CmdQueue_Basic_Unknown_Cmd,
    kGitsDiag_CmdQueue_Basic_Invalid_PhysAddr,

    /* Command: INVALL. */
    kGitsDiag_CmdQueue_Cmd_Invall_Cte_Unmapped,
    kGitsDiag_CmdQueue_Cmd_Invall_Icid_Invalid,

    /* Command: MAPC. */
    kGitsDiag_CmdQueue_Cmd_Mapc_Icid_Invalid,

    /* Command: MAPD. */
    kGitsDiag_CmdQueue_Cmd_Mapd_DevId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapd_Size_Invalid,

    /* Command: MAPI. */
    kGitsDiag_CmdQueue_Cmd_Mapi_DevId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapi_DevId_Unmapped,
    kGitsDiag_CmdQueue_Cmd_Mapi_Dte_Rd_Failed,
    kGitsDiag_CmdQueue_Cmd_Mapi_EventId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapi_IcId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapi_Ite_Wr_Failed,
    kGitsDiag_CmdQueue_Cmd_Mapi_Lpi_Invalid,

    /* Command: MAPTI. */
    kGitsDiag_CmdQueue_Cmd_Mapti_DevId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapti_DevId_Unmapped,
    kGitsDiag_CmdQueue_Cmd_Mapti_Dte_Rd_Failed,
    kGitsDiag_CmdQueue_Cmd_Mapti_EventId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapti_IcId_Invalid,
    kGitsDiag_CmdQueue_Cmd_Mapti_Ite_Wr_Failed,
    kGitsDiag_CmdQueue_Cmd_Mapti_Lpi_Invalid,

    kGitsDiag_End,
} GITSDIAG;
AssertCompileSize(GITSDIAG, 4);

#if 0
typedef struct GITSITE
{
    uint32_t        uDevId;
    uint32_t        uEventId;
    uint16_t        uIntId;
    uint8_t         uIcId;
    uint8_t         afPadding;
} GITSITE;
AssertCompileSizeAlignment(GITSITE, 4);

/**
 * Device Table Entry (DTE).
 */
#pragma pack(1)
typedef struct GITSDTE
{
    /** Whether this entry is valid. */
    uint8_t         fValid;
    /** Padding. */
    uint8_t         afPadding;
    /** The index of the cached interrupt translation table. */
    uint16_t        idxItt;
    /** The device ID. */
    uint32_t        uDevId;
    /** The physical address of the interrupt translation table. */
    RTGCPHYS        GCPhysItt;
    /** The size of the interrupt translation table in bytes. */
    uint32_t        cbItt;
} GITSDTE;
#pragma pack()
/** Pointer to a GITS device table entry. */
typedef GITSDTE *PGITSDTE;
/** Pointer to a const GITS device table entry. */
typedef GITSDTE const *PCGITSDTE;
AssertCompileSize(GITSDTE, 20);
#endif

typedef struct GITSCTE
{
    VMCPUID         idTargetCpu;
} GITSCTE;
AssertCompileSizeAlignment(GITSCTE, 4);

/** Number of supported device ID bits. */
#define GITS_DEV_ID_BITS                16
/** The last valid device ID value. */
#define GITS_DEV_ID_LAST                UINT16_MAX

/** Number of supported event ID bits. */
#define GITS_EVENT_ID_BITS              16
/** The last valid event ID value. */
#define GITS_EVENT_ID_LAST              UINT16_MAX

/** Number of entries in the LPI map cache. */
#define GITS_LPI_MAP_CACHE_COUNT        32

/**
 * GITS LPI map.
 * Maps device ID/event ID combinations to pINTID and target CPUs. Using this cache
 * avoids expensive guest memory accesses.
 *
 * This is a structure of arrays rather than an array of structures since we
 * prioritize search performance over modifying the cache. Searching elements by
 * iterating @c uDevIdEventId is faster as they would trample far fewer cache lines.
 */
typedef struct GITSLPIMAP
{
    /** The device ID (low) and event ID (high). */
    RTUINT32U       uDevIdEventId[GITS_LPI_MAP_CACHE_COUNT];
    /** The interrupt collection ID of the LPI. */
    uint16_t        uIcId[GITS_LPI_MAP_CACHE_COUNT];
    /** The physical interrupt ID of the LPI. */
    uint16_t        uIntId[GITS_LPI_MAP_CACHE_COUNT];
} GITSLPIMAP;
/** Pointer to GITS LPI map. */
typedef GITSLPIMAP *PGITSLPIMAP;
/** Pointer to a const GITS LPI map. */
typedef GITSLPIMAP const *PCGITSLPIMAP;
AssertCompileSizeAlignment(GITSLPIMAP, 4);
AssertCompileMemberSize(GITSLPIMAP, uDevIdEventId, GITS_LPI_MAP_CACHE_COUNT * (GITS_DEV_ID_BITS + GITS_EVENT_ID_BITS) / 8);

/**
 * GITS LPI map entry.
 */
typedef struct GITSLPIMAPENTRY
{
    /** The device ID (low) and event ID (high). */
    RTUINT32U       uDevIdEventId;
    /** The physical interrupt ID of the LPI. */
    uint16_t        uIntId;
    /** The interrupt collection ID of the LPI. */
    uint16_t        uIcId;
} GITSLPIMAPENTRY;
/** Pointer to GITS LPI map entry. */
typedef GITSLPIMAPENTRY *PGITSLPIMAPENTRY;
/** Pointer to a const GITS LPI map entry. */
typedef GITSLPIMAPENTRY const *PCGITSLPIMAPENTRY;
AssertCompileSizeAlignment(GITSLPIMAPENTRY, 4);
AssertCompileMemberSize(GITSLPIMAPENTRY, uDevIdEventId, (GITS_DEV_ID_BITS + GITS_EVENT_ID_BITS) / 8);

/**
 * The GITS device state.
 */
typedef struct GITSDEV
{
    /** @name Control registers.
     * @{ */
    /** The ITS control register (GITS_CTLR). */
    uint32_t                uCtrlReg;
    /** Implmentation-specific error diagnostic. */
    GITSDIAG                enmDiag;
    /** The ITS type register (GITS_TYPER). */
    RTUINT64U               uTypeReg;
    /** The ITS table descriptor registers (GITS_BASER<n>). */
    RTUINT64U               aItsTableRegs[8];
    /** The ITS command queue base registers (GITS_CBASER). */
    RTUINT64U               uCmdBaseReg;
    /** The ITS command read register (GITS_CREADR). */
    uint32_t                uCmdReadReg;
    /** The ITS command write register (GITS_CWRITER). */
    uint32_t                uCmdWriteReg;
    /** @} */

    /** @name Interrupt translation space.
     * @{ */
    /** @} */

    /** @name Command queue.
     * @{ */
    /** The command-queue thread. */
    R3PTRTYPE(PPDMTHREAD)   pCmdQueueThread;
    /** The event semaphore the command-queue thread waits on. */
    SUPSEMEVENT             hEvtCmdQueue;
    /** Number of errors while processing commands (resets on VM reset). */
    uint64_t                cCmdQueueErrors;
    /** @} */

    /** @name Tables.
     * @{
     */
    /** The collection table. */
    GITSCTE                 aCtes[255];
    /** @} */

    /** @name ITS cache.
     * @{ */
    GITSLPIMAP              LpiMap;
    /** Index of the entry to use while adding an entry to the LPI map cache. */
    uint32_t                idxLpiMap;
    /** Number of valid items in the LPI map cache. */
    uint32_t                cLpiMap;
    /** @} */

    /** @name Configurables.
     * @{ */
    /** The ITS architecture (GITS_PIDR2.ArchRev). */
    uint8_t                 uArchRev;
    /** Padding. */
    uint8_t                 afPadding0[7];
    /** @} */

    /** @name Statistics.
     * @{ */
#ifdef VBOX_WITH_STATISTICS
    STAMCOUNTER             StatCmdMapd;
    STAMCOUNTER             StatCmdMapc;
    STAMCOUNTER             StatCmdMapi;
    STAMCOUNTER             StatCmdMapti;
    STAMCOUNTER             StatCmdSync;
    STAMCOUNTER             StatCmdInv;
    STAMCOUNTER             StatCmdInvall;
    STAMCOUNTER             StatLpiCacheHit;
    STAMCOUNTER             StatLpiCacheMiss;
    STAMCOUNTER             StatLpiCacheAdd;
#endif
    /** @} */
} GITSDEV;
/** Pointer to a GITS device. */
typedef GITSDEV *PGITSDEV;
/** Pointer to a const GITS device. */
typedef GITSDEV const *PCGITSDEV;
AssertCompileSizeAlignment(GITSDEV, 8);
AssertCompileMemberAlignment(GITSDEV, aItsTableRegs, 8);
AssertCompileMemberAlignment(GITSDEV, uCmdReadReg, 4);
AssertCompileMemberAlignment(GITSDEV, uCmdWriteReg, 4);
AssertCompileMemberAlignment(GITSDEV, hEvtCmdQueue, 4);
AssertCompileMemberAlignment(GITSDEV, aCtes, 4);
AssertCompileMemberAlignment(GITSDEV, uArchRev, 4);
AssertCompileMemberSize(GITSDEV, aCtes, RT_ELEMENTS(GITSDEV::aCtes) * sizeof(GITSCTE));

DECLHIDDEN(void)            gitsInit(PGITSDEV pGitsDev);
DECLHIDDEN(void)            gitsLpiSet(PVMCC pVM, PPDMDEVINS pDevIns, PGITSDEV pGitsDev, uint32_t uDevId, uint32_t uEventId,
                                       bool fAsserted);
DECLHIDDEN(uint64_t)        gitsMmioReadCtrl(PCGITSDEV pGitsDev, uint16_t offReg, unsigned cb);
DECLHIDDEN(uint64_t)        gitsMmioReadTranslate(PCGITSDEV pGitsDev, uint16_t offReg, unsigned cb);
DECLHIDDEN(void)            gitsMmioWriteCtrl(PPDMDEVINS pDevIns, PGITSDEV pGitsDev, uint16_t offReg, uint64_t uValue, unsigned cb);
DECLHIDDEN(void)            gitsMmioWriteTranslate(PGITSDEV pGitsDev, uint16_t offReg, uint64_t uValue, unsigned cb);

DECLHIDDEN(void)            gitsLpiCacheInvalidateAll(PGITSDEV pGitsDev);

#ifdef IN_RING3
DECL_HIDDEN_CALLBACK(void)  gitsR3DbgInfo(PCGITSDEV pGitsDev, PCDBGFINFOHLP pHlp);
DECLHIDDEN(int)             gitsR3CmdQueueProcess(PCVMCC pVM, PPDMDEVINS pDevIns, PGITSDEV pGitsDev, void *pvBuf, uint32_t cbBuf);
#endif

#ifdef LOG_ENABLED
DECLHIDDEN(const char *)    gitsGetCtrlRegDescription(uint16_t offReg);
DECLHIDDEN(const char *)    gitsGetTranslationRegDescription(uint16_t offReg);
#endif

/** @} */

#endif /* !VMM_INCLUDED_SRC_include_GITSInternal_h */

