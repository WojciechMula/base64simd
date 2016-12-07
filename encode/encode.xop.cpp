#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace xop {

#define packed_dword(x) _mm_set1_epi32(x)
#define packed_word(x) _mm_set1_epi16(x)
#define packed_byte(x) _mm_set1_epi8(char(x))

        template <typename LOOKUP_FN>
        void encode(LOOKUP_FN lookup, uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const __m128i shuf = _mm_setr_epi8(
                0x00, 0x01, 0x01, 0x02,
                0x03, 0x04, 0x04, 0x05,
                0x06, 0x07, 0x07, 0x08,
                0x09, 0x0a, 0x0a, 0x0b
            );

            for (size_t i = 0; i < bytes; i += 4*3) {
                // input = [xxxx|DDDC|CCBB|BAAA]
                __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));

                // Bytes from groups A, B and C are needed in separate 32-bit lanes,
                // but the middle byte of each group is duplicated.
                in = _mm_shuffle_epi8(in, shuf);

                // lane = [dddd_ddcc|cccc_bbbb|cccc_bbbb|bbaa_aaaa]
                //                   ^^^^^^^^^ ^^^^^^^^^
                //                       duplicate

                // t0   = [0000_dddd|ddcc_cccc|cccc_bbbb|bbaa_aaaa] -- shift odd words
                //                     ^^^^^^^             ^^^^^^^
                //                a and c are on the correct positions
                const __m128i t0 = _mm_shl_epi16(in, packed_word(0x0400));

                // t1   = [00dd_dddd|cccc_cc00|ccbb_bbbb| aaaa_aa00]
                //           ^^^^^^^             ^^^^^^^
                const __m128i t1 = _mm_srli_epi16(t0, 4);

                // t2   = [??dd_dddd|??cc_cccc|??bb_bbbb|??aa_aaaa] -- '?' denotes "garbage"
                const __m128i t2 = _mm_cmov_si128(t1, t0, packed_word(0x1f00));

                // t3   = [00dd_dddd|00cc_cccc|00bb_bbbb|00aa_aaaa] -- filter out garbage
                const __m128i t3 = _mm_and_si128(t2, packed_word(0x1f1f));

                const auto result = lookup(t3);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                out += 16;
            }
        }


    #undef packed_dword
    #undef packed_byte

    } // namespace xop

} // namespace base64
