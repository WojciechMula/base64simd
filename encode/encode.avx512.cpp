#include <immintrin.h>
#include <x86intrin.h>

#include <cstdint>

namespace base64 {

    namespace avx512 {

        template <typename LOOKUP_FN, typename UNPACK_FN>
        void encode_load_gather(LOOKUP_FN lookup, UNPACK_FN unpack, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const __m512i input_offsets = _mm512_setr_epi32(
                 0*3,  1*3,  2*3,  3*3,
                 4*3,  5*3,  6*3,  7*3,
                 8*3,  9*3, 10*3, 11*3,
                12*3, 13*3, 14*3, 15*3
            );

            for (size_t i = 0; i < bytes; i += 4 * 12) {
                // load bytes
                const __m512i in = _mm512_i32gather_epi32(input_offsets, (const int*)(input + i), 1);
                const __m512i indices = unpack(in);

                // do lookup
                const __m512i result = lookup(indices);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), result);

                out += 64;
            }
        }


        template <typename LOOKUP_FN, typename UNPACK_FN>
        void encode(LOOKUP_FN lookup, UNPACK_FN unpack, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const uint8_t BIT_MERGE = 0xac;

            for (size_t i = 0; i < bytes; i += 4 * 12) {
                // load input 4 x 12 bytes, layout of 12-byte subarray:
                // tmp1 = [D2 D1 D0 C2|C1 C0 B2 B1|B0 A2 A1 A0] x 4, plus unused 16 bytes
                //           dword 2     dword 1     dword 0
                const __m512i tmp1 = _mm512_loadu_si512(input + i);

                // place each 12-byte subarray in seprate 128-bit lane
                // Note that dword 1 is duplicated
                // tmp2 = [D2 D1 D0 C2|C1 C0 B2 B1|C1 C0 B2 B1|B0 A2 A1 A0] x 4
                const __m512i tmp2 = _mm512_permutexvar_epi32(
                    _mm512_set_epi32(11, 10, 10, 9, 8, 7, 7, 6, 5, 4, 4, 3, 2, 1, 1, 0),
                    tmp1
                );

                // tmp3 = [.. .. D2 D1|D0 C2 C1 C0|C1 C0 B2 B1|B0 A2 A1 A0] x 4
                // 24-byte bundles C and A are now on desired positions
                const __m512i tmp3 = _mm512_mask_srli_epi64(tmp2, 0xaa, tmp2, 16);

                // tmp4 = [.. D2 D1 D0|C2 C1 C0 ..|C0 B2 B1 B0|A2 A1 A0 ..] x 4
                // 24-byte bundles D and B are now on desired positions
                const __m512i tmp4 = _mm512_slli_epi64(tmp3, 8);
                
                // tmp5 = [.. D2 D1 D0|.. C2 C1 C0|C0 B2 B1 B0|.. A2 A1 A0] x 4
                // all 24-byte bundles merged
                // Note: the 3rd byte of each dword will be ommited by a lookup procedure,
                //       thus no masking is needed
                const __m512i tmp5 = _mm512_ternarylogic_epi64(_mm512_set1_epi64(0x00ffffff), tmp4, tmp3, BIT_MERGE);

                const __m512i indices = unpack(tmp5);

                // do lookup
                const __m512i result = lookup(indices);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), result);

                out += 64;
            }
        }

    } // namespace avx512

} // namespace base64
