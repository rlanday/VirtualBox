/** @file
 * Safe way to include dispmprt.h (DDK).
 */

/*
 * Copyright (C) 2016-2024 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_nt_dispmprt_h
#define IPRT_INCLUDED_nt_dispmprt_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4668) /* warning C4668: 'DBG' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
# ifndef __cplusplus
#  pragma warning(disable:4255) /* video.h(1776) : warning C4255: 'VideoPortGetCurrentIrql' : no function prototype given: converting '()' to '(void)' */
# endif
# if _MSC_VER >= 1930 /*RT_MSC_VER_VC143*/
#  pragma warning(disable:5249) /* d3dkmdt.h(551): warning C5249: '_D3DKMDT_VIDEO_SIGNAL_INFO::<unnamed-tag>::ScanLineOrdering' of type 'D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING' has named enumerators with values that cannot be represented in the given bit field width of '3'. */
# endif
#endif

RT_C_DECLS_BEGIN
#include <dispmprt.h>
RT_C_DECLS_END

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif /* !IPRT_INCLUDED_nt_dispmprt_h */

