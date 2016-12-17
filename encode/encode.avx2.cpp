#include <immintrin.h>
#include <x86intrin.h>

#include <cstdint>

// definition copied from http://stackoverflow.com/questions/32630458/setting-m256i-to-the-value-of-two-m128i-values
// thank you Paul! (http://stackoverflow.com/users/253056/paul-r)
#define _mm256_set_m128i(v0, v1)  _mm256_insertf128_si256(_mm256_castsi128_si256(v1), (v0), 1)


namespace base64 {

    namespace avx2 {

#define packed_dword(x) _mm256_set1_epi32(x)

        template <typename LOOKUP_FN>
        void encode(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 2*4*3) {
#if 1
                // lo = [xxxx|DDDC|CCBB|BAAA]
                // hi = [xxxx|HHHG|GGFF|FEEE]
                const __m128i lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));
                const __m128i hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3));

                // bytes from groups A, B and C are needed in separate 32-bit lanes
                // in = [0HHH|0GGG|0FFF|0EEE[0DDD|0CCC|0BBB|0AAA]
                const __m256i shuf = _mm256_set_epi8(
                    10, 11, 9, 10,
                     7,  8, 6,  7,
                     4,  5, 3,  4,
                     1,  2, 0,  1,

                    10, 11, 9, 10,
                     7,  8, 6,  7,
                     4,  5, 3,  4,
                     1,  2, 0,  1
                );

                __m256i in = _mm256_shuffle_epi8(_mm256_set_m128i(hi, lo), shuf);
#else
                // just one memory load, however it read 4-bytes off on the first iteration

                const __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i - 4));
                // data = [????|HHHG|GGFF|FEEE|DDDC|CCBB|BAAA|????]
                //            256-bit lane    |   256-bit lane

                // bytes from groups A...G are placed in separate 32-bit lanes
                // in = [0HHH|0GGG|0FFF|0EEE|0DDD|0CCC|0BBB|0AAA]
                const __m256i shuf = _mm256_setr_epi8(
                    4 + 0x00, 4 + 0x01, 4 + 0x02, char(0xff),
                    4 + 0x03, 4 + 0x04, 4 + 0x05, char(0xff),
                    4 + 0x06, 4 + 0x07, 4 + 0x08, char(0xff),
                    4 + 0x09, 4 + 0x0a, 4 + 0x0b, char(0xff),

                        0x00,     0x01,     0x02, char(0xff),
                        0x03,     0x04,     0x05, char(0xff),
                        0x06,     0x07,     0x08, char(0xff),
                        0x09,     0x0a,     0x0b, char(0xff)
                );

                __m256i in = _mm256_shuffle_epi8(data, shuf);
#endif

                // this part is well commented in encode.sse.cpp

                const __m256i t0 = _mm256_and_si256(in, _mm256_set1_epi32(0x0fc0fc00));
                const __m256i t1 = _mm256_mulhi_epu16(t0, _mm256_set1_epi32(0x04000040));
                const __m256i t2 = _mm256_and_si256(in, _mm256_set1_epi32(0x003f03f0));
                const __m256i t3 = _mm256_mullo_epi16(t2, _mm256_set1_epi32(0x01000010));
                const __m256i indices = _mm256_or_si256(t1, t3);

                const auto result = lookup(indices);

                _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), result);
                out += 32;
            }
        }


        template <typename LOOKUP_FN>
        void encode_unrolled(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const __m256i shuf = _mm256_set_epi8(
                10, 11, 9, 10,
                 7,  8, 6,  7,
                 4,  5, 3,  4,
                 1,  2, 0,  1,

                10, 11, 9, 10,
                 7,  8, 6,  7,
                 4,  5, 3,  4,
                 1,  2, 0,  1
            );

            for (size_t i = 0; i < bytes; i += 2*4*3 * 4) {

                const __m128i lo0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*0));
                const __m128i hi0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*1));
                const __m128i lo1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*2));
                const __m128i hi1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*3));
                const __m128i lo2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*4));
                const __m128i hi2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*5));
                const __m128i lo3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*6));
                const __m128i hi3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3*7));

                __m256i in0 = _mm256_shuffle_epi8(_mm256_set_m128i(hi0, lo0), shuf);
                __m256i in1 = _mm256_shuffle_epi8(_mm256_set_m128i(hi1, lo1), shuf);
                __m256i in2 = _mm256_shuffle_epi8(_mm256_set_m128i(hi2, lo2), shuf);
                __m256i in3 = _mm256_shuffle_epi8(_mm256_set_m128i(hi3, lo3), shuf);

                const __m256i t0_0 = _mm256_and_si256(in0, _mm256_set1_epi32(0x0fc0fc00));
                const __m256i t0_1 = _mm256_and_si256(in1, _mm256_set1_epi32(0x0fc0fc00));
                const __m256i t0_2 = _mm256_and_si256(in2, _mm256_set1_epi32(0x0fc0fc00));
                const __m256i t0_3 = _mm256_and_si256(in3, _mm256_set1_epi32(0x0fc0fc00));

                const __m256i t1_0 = _mm256_mulhi_epu16(t0_0, _mm256_set1_epi32(0x04000040));
                const __m256i t1_1 = _mm256_mulhi_epu16(t0_1, _mm256_set1_epi32(0x04000040));
                const __m256i t1_2 = _mm256_mulhi_epu16(t0_2, _mm256_set1_epi32(0x04000040));
                const __m256i t1_3 = _mm256_mulhi_epu16(t0_3, _mm256_set1_epi32(0x04000040));

                const __m256i t2_0 = _mm256_and_si256(in0, _mm256_set1_epi32(0x003f03f0));
                const __m256i t2_1 = _mm256_and_si256(in1, _mm256_set1_epi32(0x003f03f0));
                const __m256i t2_2 = _mm256_and_si256(in2, _mm256_set1_epi32(0x003f03f0));
                const __m256i t2_3 = _mm256_and_si256(in3, _mm256_set1_epi32(0x003f03f0));

                const __m256i t3_0 = _mm256_mullo_epi16(t2_0, _mm256_set1_epi32(0x01000010));
                const __m256i t3_1 = _mm256_mullo_epi16(t2_1, _mm256_set1_epi32(0x01000010));
                const __m256i t3_2 = _mm256_mullo_epi16(t2_2, _mm256_set1_epi32(0x01000010));
                const __m256i t3_3 = _mm256_mullo_epi16(t2_3, _mm256_set1_epi32(0x01000010));

                const __m256i input0 = _mm256_or_si256(t1_0, t3_0);
                const __m256i input1 = _mm256_or_si256(t1_1, t3_1);
                const __m256i input2 = _mm256_or_si256(t1_2, t3_2);
                const __m256i input3 = _mm256_or_si256(t1_3, t3_3);

                {
                    const auto result = lookup(input0);

                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), result);
                    out += 32;
                }
                {
                    const auto result = lookup(input1);

                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), result);
                    out += 32;
                }
                {
                    const auto result = lookup(input2);

                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), result);
                    out += 32;
                }
                {
                    const auto result = lookup(input3);

                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), result);
                    out += 32;
                }
            }
        }


#if defined(HAVE_BMI2_INSTRUCTIONS)
        template <typename LOOKUP_FN>
        void encode_bmi2(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 4*6) {

                const uint64_t d0 = *reinterpret_cast<const uint64_t*>(input + i + 0*6);
                const uint64_t d1 = *reinterpret_cast<const uint64_t*>(input + i + 1*6);
                const uint64_t d2 = *reinterpret_cast<const uint64_t*>(input + i + 2*6);
                const uint64_t d3 = *reinterpret_cast<const uint64_t*>(input + i + 3*6);
                const uint64_t t0 = __builtin_bswap64(d0) >> 16;
                const uint64_t t1 = __builtin_bswap64(d1) >> 16;
                const uint64_t t2 = __builtin_bswap64(d2) >> 16;
                const uint64_t t3 = __builtin_bswap64(d3) >> 16;
                const uint64_t expanded_0 = _pdep_u64(t0, 0x3f3f3f3f3f3f3f3flu);
                const uint64_t expanded_1 = _pdep_u64(t1, 0x3f3f3f3f3f3f3f3flu);
                const uint64_t expanded_2 = _pdep_u64(t2, 0x3f3f3f3f3f3f3f3flu);
                const uint64_t expanded_3 = _pdep_u64(t3, 0x3f3f3f3f3f3f3f3flu);

                const __m256i indices = _mm256_setr_epi32(
                    __builtin_bswap32(expanded_0 >> 32),
                    __builtin_bswap32(uint32_t(expanded_0)),
                    __builtin_bswap32(expanded_1 >> 32),
                    __builtin_bswap32(uint32_t(expanded_1)),
                    __builtin_bswap32(expanded_2 >> 32),
                    __builtin_bswap32(uint32_t(expanded_2)),
                    __builtin_bswap32(expanded_3 >> 32),
                    __builtin_bswap32(uint32_t(expanded_3))
                );

                const auto result = lookup(indices);

                _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), result);
                out += 32;
            }
        }
#endif

    #undef packed_dword

    } // namespace avx2

} // namespace base64
