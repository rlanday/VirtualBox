/* $Id$ */
/** @file
 * IEM - Interpreted Execution Manager - IEM_MC_XXX, ARMv8 target.
 */

/*
 * Copyright (C) 2011-2025 Oracle and/or its affiliates.
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

#ifndef VMM_INCLUDED_SRC_VMMAll_target_armv8_IEMMc_armv8_h
#define VMM_INCLUDED_SRC_VMMAll_target_armv8_IEMMc_armv8_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/** @name   "Microcode" macros, ARMv8 specifics and overrides.
 * @{
 */

 /** Set 64-bit PC to uAddr (local) and PSTATE.BTYPE to a_uBType (constant). */
#define IEM_MC_BRANCH_TO_WITH_BTYPE_AND_FINISH(a_uNewPc, a_uBType); \
    return iemRegPcA64BranchToAndFinishClearingFlags((pVCpu), (a_uNewPc), (a_uBType))

#define IEM_MC_FETCH_GREG_SP_U32(a_u32Dst, a_iGReg)     (a_u32Dst) = iemGRegFetchU32(pVCpu, (a_iGReg), true /*fSp*/)
#define IEM_MC_FETCH_GREG_SP_U64(a_u64Dst, a_iGReg)     (a_u64Dst) = iemGRegFetchU64(pVCpu, (a_iGReg), true /*fSp*/)

#define IEM_MC_STORE_GREG_SP_U32(a_iGReg, a_u32Value)   iemGRegStoreU32(pVCpu, (a_iGReg), (a_u32Value), true /*fSp*/)
#define IEM_MC_STORE_GREG_SP_U64(a_iGReg, a_u64Value)   iemGRegStoreU64(pVCpu, (a_iGReg), (a_u64Value), true /*fSp*/)

#define IEM_MC_A64_SUBS_U64(a_uDifference, a_uMinuend, a_uSubtrahend) \
    (a_uDifference) = iemMcA64SubsU64(a_uMinuend, a_uSubtrahend, &pVCpu->cpum.GstCtx.fPState)


/** Helper that implements IEM_MC_A64_SUBS_U64. */
DECLINLINE(uint64_t) iemMcA64SubsU64(uint64_t uMinuend, uint64_t uSubtrahend, uint64_t *pfPState)
{
    uint64_t const uDiff     = uMinuend - uSubtrahend;
    uint64_t const fNewFlags = ((uDiff >> (63 - ARMV8_SPSR_EL2_AARCH64_N_BIT)) & ARMV8_SPSR_EL2_AARCH64_N)
                             | (uDiff                  ? 0 : ARMV8_SPSR_EL2_AARCH64_Z)
                             | (uMinuend < uSubtrahend ? 0 : ARMV8_SPSR_EL2_AARCH64_C) /* inverted for subtractions */
                             | (((((uMinuend ^ uSubtrahend) & (uDiff ^ uMinuend)) >> 63) & 1) << ARMV8_SPSR_EL2_AARCH64_V_BIT);
    *pfPState = (*pfPState & ~ARMV8_SPSR_EL2_AARCH64_NZCV) | fNewFlags;
    return uDiff;
}

/** @}  */

#endif /* !VMM_INCLUDED_SRC_VMMAll_target_armv8_IEMMc_armv8_h */

