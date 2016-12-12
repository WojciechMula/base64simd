#include <cstdint>
#include <cassert>

#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace avx512 {

        template <typename FN_LOOKUP, typename FN_PACK>
        void decode_store_scatter(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 64 == 0);

            uint8_t* out = output;

            const __m512i store_offsets = _mm512_setr_epi32(
                 0*3,  1*3,  2*3,  3*3,
                 4*3,  5*3,  6*3,  7*3,
                 8*3,  9*3, 10*3, 11*3,
                12*3, 13*3, 14*3, 15*3
            );

            for (size_t i=0; i < size; i += 64) {

                __m512i in = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));
                __m512i values;

                try {
                    values = lookup(in);
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                const __m512i packed = pack(values);

                _mm512_i32scatter_epi32(reinterpret_cast<int*>(out), store_offsets, packed, 1);

                out += 48;
            }
        }

        template <typename FN_LOOKUP, typename FN_PACK>
        void decode(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 64 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 64) {

                __m512i in = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));
                __m512i values;

                try {
                    values = lookup(in);
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                const __m512i packed = pack(values);

                //                 |  32 bits  |
                // packed        = [.. D2 D1 D0|.. C2 C1 C0|.. B2 B1 B0|.. A2 A1 A0] x 4 (four 128-bit lanes)
                // index            3           2           1           0

                // t1            = [.. D2 D1 D0|C2 C1 C0 ..|.. B2 B1 B0|A2 A1 A0 ..] x 4
                // shift even dwords 8 bits left
                const __m512i t1 = _mm512_mask_slli_epi32(packed, 0x5555, packed, 8);

                // t2            = [.. .. .. ..|D2 D1 D0 C2|.. .. B2 B1|B0 A2 A1 A0] x 4
                //                              ^^^^^^^^^^^       ^^^^^ ^^^^^^^^^^^
                //                                     correct  position
                const __m512i s2 = _mm512_setr_epi64(8, 24, 8, 24, 8, 24, 8, 24);
                const __m512i t2 = _mm512_srlv_epi64(t1, s2);

                // t3            = [E2 E1 E0 ..|.. D2 D1 D0|C2 C1 C0 ..|.. B2 B1 B0] x 4
                //                                             ^^^^^
                //                                             only these bytes are needed
                const __m512i t3 = _mm512_alignr_epi32(t1, t1, 1);

                // t4            = [.. .. .. ..|.. .. .. ..|C1 C0 .. ..|.. .. .. ..] x 4
                //                                          ^^^^^
                //                                          only these bytes are needed
                const __m512i t4 = _mm512_maskz_slli_epi32(0x2222, t3, 8);

                // t5            = [.. .. .. ..|D2 D1 D0 C2|C1 C0 B2 B1|B0 A2 A1 A0] x 4
                //
                // this works providing that the pack step zeroes 3rd bytes of packed vector
                const __m512i t5 = _mm512_or_si512(t4, t2);

                // shuffle bytes
                const __m512i s6 = _mm512_setr_epi32(
                     0,  1,  2,
                     4,  5,  6,
                     8,  9, 10,
                    12, 13, 14,
                    // unused
                     0,  0,  0, 0);

                const __m512i t6 = _mm512_permutexvar_epi32(s6, t5);

                // Note: overwritting is faster than masked stored
                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), t6);
                //_mm512_mask_storeu_epi32(reinterpret_cast<__m512i*>(out), 0x0fff, t6);

                out += 48;
            }
        }

    } // namespace avx512bw

} // namespace base64
