#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace avx512 {


#define packed_dword(x) _mm512_set1_epi32(x)
#define masked(x, mask) _mm512_and_si512(x, packed_dword(mask))

        template <int shift, uint32_t mask>
        __m512i merge(__m512i target, __m512i src) {
            __m512i shifted;
            if (shift > 0) {
                shifted = _mm512_srli_epi32(src, shift);
            } else {
                shifted = _mm512_slli_epi32(src, -shift);
            }

            const static uint8_t MERGE_BITS = 0xca;
            return _mm512_ternarylogic_epi32(_mm512_set1_epi32(mask), shifted, target, MERGE_BITS);
        }


        __m512i pack_improved(const __m512i in) {

            // in = |00dddddd|00cccccc|00bbbbbb|00aaaaaa|

            // t0 = |00000000|00000000|00000000|aaaaaa00|
            const __m512i t0 = _mm512_slli_epi32(masked(in, 0x0000003f), 2);

            // t1 = |00000000|00000000|00000000|aaaaaabb|
            const __m512i t1 = merge<12, 0x00000003>(t0, in);

            // t2 = |00000000|00000000|bbbb0000|aaaaaabb|
            const __m512i t2 = merge<-4, 0x0000f000>(t1, in);

            // t3 = |00000000|00000000|bbbbcccc|aaaaaabb|
            const __m512i t3 = merge<10, 0x00000f00>(t2, in);

            // t4 = |00000000|cc000000|bbbbcccc|aaaaaabb|
            const __m512i t4 = merge<-6, 0x00c00000>(t3, in);

            // t5 = |00000000|ccdddddd|bbbbcccc|aaaaaabb|
            const __m512i t5 = merge< 8, 0x003f0000>(t4, in);

            return t5;
        }


        __m512i pack_identity(const __m512i values) {
            return values;
        }

#undef packed_dword
#undef masked

    } // namespace avx512

} // namespace base64
