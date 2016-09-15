#include <immintrin.h>
#include <x86intrin.h>

#include <cstdint>
#include "../debug_dump.cpp"

//#define GATHER_ASSISTED_LOAD

namespace base64 {

    namespace avx512 {

        template <typename LOOKUP_FN, typename UNPACK_FN>
        void encode(LOOKUP_FN lookup, UNPACK_FN unpack, uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

#ifdef GATHER_ASSISTED_LOAD
            static const uint32_t input_offsets[16] = {
                 0*3,  1*3,  2*3,  3*3,
                 4*3,  5*3,  6*3,  7*3,
                 8*3,  9*3, 10*3, 11*3,
                12*3, 13*3, 14*3, 15*3
            };

            const __m512i input_gather = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input_offsets));
#endif
            for (size_t i = 0; i < bytes; i += 4 * 12) {
                // load bytes
#ifdef GATHER_ASSISTED_LOAD
                const __m512i in = _mm512_i32gather_epi32(input_gather, (const int*)(input + i), 1);
#else
                const __m512i tmp1 = _mm512_loadu_si512(input + i);
                const __m512i tmp2 = _mm512_permutexvar_epi32(
                    _mm512_set_epi32(11, 10, 10, 9, 8, 7, 7, 6, 5, 4, 4, 3, 2, 1, 1, 0),
                    tmp1
                );
                const __m512i tmp3 = _mm512_mask_srli_epi64(tmp2, 0xaa, tmp2, 16);
                const __m512i tmp4 = _mm512_slli_epi64(tmp3, 8);
                const __m512i tmp5 = _mm512_ternarylogic_epi64(_mm512_set1_epi64(0x00ffffff), tmp4, tmp3, 0xac);

                const __m512i in = tmp5;
#endif // defined(GATHER_ASSISTED_LOAD)

                const __m512i indices = unpack(in);

                // do lookup
                const __m512i result = lookup(indices);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), result);

                out += 64;
            }
        }

    } // namespace avx512

} // namespace base64
