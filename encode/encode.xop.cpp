#include <immintrin.h>
#include <x86intrin.h>


namespace base64 {

    namespace xop {

#define packed_dword(x) _mm_set1_epi32(x)
#define packed_word(x) _mm_set1_epi16(x)

        template <typename LOOKUP_FN>
        void encode(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const __m128i shuf = _mm_set_epi8(
                10, 11, 9, 10,
                 7,  8, 6,  7,
                 4,  5, 3,  4,
                 1,  2, 0,  1
            );

            for (size_t i = 0; i < bytes; i += 4*3) {
                // input = [xxxx|DDDC|CCBB|BAAA]
                __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));

                in = _mm_shuffle_epi8(in, shuf);

                // lane = [bbbb_cccc|ccdd_dddd|aaaa_aabb|bbbb_cccc]

                // t0   = [0000_00bb|bbcc_cccc|cccc_bbbb|bbaa_aaaa] -- (c >> 6, a >> 10)
                //                     ^^^^^^^             ^^^^^^^
                //                a and c are on the correct positions
                const __m128i t0 = _mm_shl_epi16(in, packed_dword(0xfffafff6));

                // t1   = [0000_0000|00cc_cccc|0000_0000|00aa_aaaa] -- left a and c fields
                const __m128i t1 = _mm_and_si128(t0, packed_word(0x003f));

                // t2   = [ccdd_dddd|0000_0000|ccbb_bbbb|aaaa_aa00] -- (d << 8, b << 4)
                //           ^^^^^^^             ^^^^^^^
                const __m128i t2 = _mm_shl_epi16(in, packed_dword(0x00080004));

                // t3   = [00dd_dddd|00cc_cccc|00bb_bbbb|00aa_aaaa]
                const __m128i indices = _mm_cmov_si128(t2, t1, packed_word(0x3f00));

                const auto result = lookup(indices);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                out += 16;
            }
        }

    #undef packed_dword
    #undef packed_word

    } // namespace xop

} // namespace base64
