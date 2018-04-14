#define build_dword(b0, b1, b2, b3)     \
     ((uint32_t(uint8_t(b0)) << 0*8)    \
    | (uint32_t(uint8_t(b1)) << 1*8)    \
    | (uint32_t(uint8_t(b2)) << 2*8)    \
    | (uint32_t(uint8_t(b3)) << 3*8))
    
#define _mm512_set4lanes_epi8(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
    _mm512_setr4_epi32(                  \
        build_dword( b0,  b1,  b2,  b3), \
        build_dword( b4,  b5,  b6,  b7), \
        build_dword( b8,  b9, b10, b11), \
        build_dword(b12, b13, b14, b15))

#include "decode.avx512bw.cpp"
#include "lookup.avx512bw.cpp"
#include "pack.avx512bw.cpp"
