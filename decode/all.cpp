#include "decode.common.cpp"
#include "decode.scalar.cpp"
#if defined(HAVE_SSE_INSTRUCTIONS)
#   include "decoders.sse.cpp"
#endif
#if defined(HAVE_XOP_INSTRUCTIONS)
#   include "decoders.xop.cpp"
#endif
#if defined(HAVE_AVX2_INSTRUCTIONS)
#   include "decoders.avx2.cpp"
#endif
#if defined(HAVE_AVX512_INSTRUCTIONS)
#   include "decoders.avx512.cpp"
#endif
#if defined(HAVE_AVX512BW_INSTRUCTIONS)
#   include "decoders.avx512bw.cpp"
#endif
#if defined(HAVE_NEON_INSTRUCTIONS)
#   include "decoders.neon.cpp"
#endif

