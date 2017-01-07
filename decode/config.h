#pragma once

#define FORCE_INLINE inline __attribute__((always_inline))

#if (defined(HAVE_AVX512_INSTRUCTIONS) || defined(HAVE_AVX512BW_INSTRUCTIONS)) && !defined(HAVE_AVX2_INSTRUCTIONS)
#   if !defined(HAVE_AVX2_INSTRUCTIONS)
#       define HAVE_AVX2_INSTRUCTIONS 1
#   endif
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS) || defined(HAVE_XOP_INSTRUCTIONS)
#   if !defined(HAVE_SSE_INSTRUCTIONS)
#       define HAVE_SSE_INSTRUCTIONS 1
#   endif
#endif

#if !defined(HAVE_NEON_INSTRUCTIONS)
#   include <immintrin.h>
#   include <x86intrin.h>
#endif
