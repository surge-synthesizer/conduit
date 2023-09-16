/*
 * Conduit - a series of demonstration and fun plugins
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license, but the assembled program which results
 * from compiling it has GPL3 dependencies, so the total
 * program is a GPL3 program. More details to come.
 *
 * Basically before I give this to folks, document this bit and
 * replace these headers
 *
 */

#ifndef CONDUIT_SRC_CONDUIT_SHARED_SSE_INCLUDE_H
#define CONDUIT_SRC_CONDUIT_SHARED_SSE_INCLUDE_H

#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) ||                                   \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#include <pmmintrin.h>
#else
#if defined(__arm__) || defined(__aarch64__) || defined(__riscv)
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#else
#error Conduit requires either X86/SSE2 or ARM architectures.
#endif
#endif

#endif // CONDUIT_SSE_INCLUDE_H
