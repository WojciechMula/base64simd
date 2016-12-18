#include <immintrin.h>
#include <x86intrin.h>

/*
    Pack algorithm is responsible for converting from
    four 6-bit indices saved in separate bytes of 32-bit word
    into 24-bit word
*/

namespace base64 {

    namespace sse {

#define packed_dword(x) _mm_set1_epi32(x)
#define masked(x, mask) _mm_and_si128(x, _mm_set1_epi32(mask))

        __m128i pack_naive(const __m128i values) {

            // input:  [00dddddd|00cccccc|00bbbbbb|00aaaaaa]

            const __m128i ca = masked(values, 0x003f003f);
            const __m128i db = masked(values, 0x3f003f00);

            // t0   =  [0000cccc|ccdddddd|0000aaaa|aabbbbbb]
            const __m128i t0 = _mm_or_si128(
                                _mm_srli_epi32(db, 8),
                                _mm_slli_epi32(ca, 6)
                               );

            // t1   =  [dddd0000|aaaaaabb|bbbbcccc|dddddddd]
            const __m128i t1 = _mm_or_si128(
                                _mm_srli_epi32(t0, 16),
                                _mm_slli_epi32(t0, 12)
                               );

            // result: [00000000|aaaaaabb|bbbbcccc|ccdddddd]
            return masked(t1, 0x00ffffff);
        }


        __m128i pack_madd(const __m128i values) {
            // input:  [00dddddd|00cccccc|00bbbbbb|00aaaaaa]

            // merge:  [0000cccc|ccdddddd|0000aaaa|aabbbbbb]
            const __m128i merge_ab_and_bc = _mm_maddubs_epi16(values, packed_dword(0x01400140));

            // result: [00000000|aaaaaabb|bbbbcccc|ccdddddd]
            return _mm_madd_epi16(merge_ab_and_bc, packed_dword(0x00011000));
        }

#undef packed_dword
#undef masked

    } // namespace sse

} // namespace base64
