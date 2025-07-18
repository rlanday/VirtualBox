/* $Id$ */
/** @file
 * libjpeg-turbo - This contains only the stuff we can't do in Makefile.kmk.
 * The rest of the preprocessor / constant stuff is done in Makefile.kmk.
 */

/*
 * Copyright (C) 2010-2024 Oracle and/or its affiliates.
 *
 * Oracle Corporation confidential
 */

#ifndef VBOX_INCLUDED_SRC_libjpeg_turbo_3_1_0_jconfigint_h
#define VBOX_INCLUDED_SRC_libjpeg_turbo_3_1_0_jconfigint_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef _MSC_VER
# define HIDDEN                 __attribute__((visibility("hidden")))
# endif
# if defined(__clang__)
# define HIDDEN                 __attribute__((visibility("hidden")))
# endif

#ifdef _MSC_VER

# define HAVE_INTRIN_H          1
# define HAVE_BITSCANFORWARD    1
# if (SIZEOF_SIZE_T) == 8
#  define HAVE_BITSCANFORWARD64 1
# endif

# define THREAD_LOCAL           __declspec(thread)

# define INLINE                 __forceinline

#else

# if defined(RT_ARCH_ARM64)
#  define HAVE_BUILTIN_CTZL     1
# endif

# define INLINE                 __inline__ __attribute__((always_inline))

# ifdef __has_attribute
#  if __has_attribute(fallthrough)
#   define FALLTHROUGH          __attribute__((fallthrough));
#  endif
# endif

#endif

#ifndef FALLTHROUGH
# define FALLTHROUGH
#endif

#ifndef HIDDEN
# define HIDDEN
#endif

/*
 * Define BITS_IN_JSAMPLE as either
 *   8   for 8-bit sample values (the usual setting)
 *   12  for 12-bit sample values
 * Only 8 and 12 are legal data precisions for lossy JPEG according to the
 * JPEG standard, and the IJG code does not support anything else!
 */

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8      /* use 8 or 12 */
#endif

#undef C_ARITH_CODING_SUPPORTED
#undef D_ARITH_CODING_SUPPORTED
#undef WITH_SIMD

#if BITS_IN_JSAMPLE == 8

/* Support arithmetic encoding */
#define C_ARITH_CODING_SUPPORTED 1

/* Support arithmetic decoding */
#define D_ARITH_CODING_SUPPORTED 1

/* Use accelerated SIMD routines. */
#define WITH_SIMD 1

#endif

#endif /* !VBOX_INCLUDED_SRC_libjpeg_turbo_3_1_0_jconfigint_h */
