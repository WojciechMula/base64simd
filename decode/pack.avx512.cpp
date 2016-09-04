#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace avx512 {

#define packed_dword(x) _mm512_set1_epi32(x)

        // please refer to pack.avx2.cpp for comments
        __m512i pack_improved_basic(const __m512i values) {

            const __m512i bits_ac = _mm512_and_si512(values, packed_dword(0x003f003f));
            const __m512i bits_bd = _mm512_and_si512(values, packed_dword(0x3f003f00));
            const __m512i tmp     = _mm512_or_si512(bits_ac, _mm512_srli_epi32(bits_bd, 2));
            const __m512i bits_cd = _mm512_and_si512(tmp, packed_dword(0x0fff0000));
            const __m512i bits_ab = _mm512_and_si512(tmp, packed_dword(0x00000fff));

            return _mm512_or_si512(bits_ab, _mm512_srli_epi32(bits_cd, 4));
        }


        const uint8_t MERGE_BITS = 0xac;


        // the improved version using ternary logic instructions
        __m512i pack_improved(const __m512i in) {

            // a1 = 00aaaaaa|00bbbbbb|00cccccc|00dddddd
            // b1 = 0000AAAA|AA00BBBB|BB00CCCC|CC00DDDD
            // m1 = 0000AAAA|AAbbbbbb|????CCCC|CCdddddd

            const __m512i a1 = in;
            const __m512i b1 = _mm512_srli_epi32(in, 2);
            const __m512i m1 = _mm512_ternarylogic_epi32(packed_dword(0x0fc00fc0), a1, b1, MERGE_BITS);
            
            // a2 = 0000aaaa|aabbbbbb|????cccc|ccdddddd
            // b2 = 00000000|AAAAAABB|BBBB????|CCCCCCDD
            // m2 = 00000000|AAAAAABB|BBBBcccc|ccdddddd

            const __m512i a2 = m1;
            const __m512i b2 = _mm512_srli_epi32(m1, 4);
            const __m512i m2 = _mm512_ternarylogic_epi32(packed_dword(0x00fff000), a2, b2, MERGE_BITS);

            return m2;
        }


        __m512i pack_identity(const __m512i values) {
            return values;
        }

#undef packed_dword

    } // namespace avx512

} // namespace base64
