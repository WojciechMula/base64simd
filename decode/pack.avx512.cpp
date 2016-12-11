#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace avx512 {

#define packed_dword(x) _mm512_set1_epi32(x)
#define masked(x, mask) _mm512_and_si512(x, packed_dword(mask))

        const uint8_t MERGE_BITS = 0xac;

#define insert_shifted_right(trg, src, amount, mask)    \
        _mm512_ternarylogic_epi32(                      \
            packed_dword(mask),                         \
            trg,                                        \
            _mm512_srli_epi32(src, amount),             \
            MERGE_BITS)

#define insert_shifted_left(trg, src, amount, mask)     \
        _mm512_ternarylogic_epi32(                      \
            packed_dword(mask),                         \
            trg,                                        \
            _mm512_slli_epi32(src, amount),             \
            MERGE_BITS)

        __m512i pack_improved(const __m512i in) {

            // in = |00dddddd|00cccccc|00bbbbbb|00aaaaaa|

            // t0 = |00000000|00000000|00000000|aaaaaa00|
            const __m512i t0 = _mm512_slli_epi32(masked(in, 0x0000003f), 2);

            // t1 = |00000000|00000000|00000000|aaaaaabb|
            const __m512i t1 = insert_shifted_right(t0, in, 12, 0x00000003);

            // t2 = |00000000|00000000|bbbb0000|aaaaaabb|
            const __m512i t2 = insert_shifted_left (t1, in,  4, 0x0000f000);

            // t3 = |00000000|00000000|bbbbcccc|aaaaaabb|
            const __m512i t3 = insert_shifted_right(t2, in, 10, 0x00000f00);

            // t4 = |00000000|cc000000|bbbbcccc|aaaaaabb|
            const __m512i t4 = insert_shifted_left (t3, in,  6, 0x00c00000);

            // t5 = |00000000|ccdddddd|bbbbcccc|aaaaaabb|
            const __m512i t5 = insert_shifted_right(t4, in,  8, 0x003f0000);

            return t5;
        }


        __m512i pack_identity(const __m512i values) {
            return values;
        }

#undef packed_dword
#undef masked

    } // namespace avx512

} // namespace base64
