#include <cstdint>
#include <cassert>

#include <immintrin.h>
#include <x86intrin.h>


//#define SCATTER_ASSISTED_STORE
#define AVX2_ASSISTED_STORE
//#define SSE_ASSISTED_STORE

namespace base64 {

    namespace avx512 {
#ifdef SCATTER_ASSISTED_STORE
        namespace precalc {

            static __m512i scatter_offsets;
        }
#endif

        void initalize_decode() {
#ifdef SCATTER_ASSISTED_STORE
            uint32_t lookup[16];

            for (int i=0; i < 16; i++) {
                lookup[i] = i * 3;
            }

            precalc::scatter_offsets = _mm512_loadu_si512(reinterpret_cast<__m512i*>(lookup));
#endif

        }


        static const uint8_t BIT_MERGE = 0xac;


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

#ifdef SCATTER_ASSISTED_STORE
                _mm512_i32scatter_epi32(reinterpret_cast<int*>(out), precalc::scatter_offsets, packed, 1);
#endif

#ifdef SSE_ASSISTED_STORE
                const __m128i dq0 = _mm512_extracti32x4_epi32(packed, 0);
                const __m128i dq1 = _mm512_extracti32x4_epi32(packed, 1);
                const __m128i dq2 = _mm512_extracti32x4_epi32(packed, 2);
                const __m128i dq3 = _mm512_extracti32x4_epi32(packed, 3);

                const __m128i shuffle = _mm_setr_epi8(
                     0,  1,  2,
                     4,  5,  6,
                     8,  9, 10,
                    12, 13, 14,
                    char(0xff), char(0xff), char(0xff), char(0xff)
                );

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 0*12), _mm_shuffle_epi8(dq0, shuffle));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 1*12), _mm_shuffle_epi8(dq1, shuffle));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 2*12), _mm_shuffle_epi8(dq2, shuffle));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 3*12), _mm_shuffle_epi8(dq3, shuffle));
#endif

#ifdef AVX2_ASSISTED_STORE
                const __m256i v0 = _mm512_extracti64x4_epi64(packed, 0);
                const __m256i v1 = _mm512_extracti64x4_epi64(packed, 1);

                const __m256i shuf = _mm256_setr_epi8(
                       0,  1,  2,
                       4,  5,  6,
                       8,  9, 10,
                      12, 13, 14,
                      char(0xff), char(0xff), char(0xff), char(0xff),
                       0,  1,  2,
                       4,  5,  6,
                       8,  9, 10,
                      12, 13, 14,
                      char(0xff), char(0xff), char(0xff), char(0xff)
                );

                const __m256i v0s = _mm256_shuffle_epi8(v0, shuf);
                const __m256i v1s = _mm256_shuffle_epi8(v1, shuf);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 0*12), _mm256_extracti128_si256(v0s, 0));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 1*12), _mm256_extracti128_si256(v0s, 1));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 2*12), _mm256_extracti128_si256(v1s, 0));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 3*12), _mm256_extracti128_si256(v1s, 1));
#endif
                out += 48;
            }
        }

    } // namespace avx512bw

} // namespace base64
