#include <cstdint>
#include <cassert>

#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace avx2 {

        template <typename FN_LOOKUP, typename FN_PACK>
        void decode(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 32 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 32) {

                __m256i in = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));
                __m256i values;

                try {
                    values = lookup(in);
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                // input:  packed_dword([00dddddd|00cccccc|00bbbbbb|00aaaaaa] x 4)
                // merged: packed_dword([00000000|ddddddcc|ccccbbbb|bbaaaaaa] x 4)

                const __m256i merged = pack(values);

                // merged = packed_byte([0XXX|0YYY|0ZZZ|0WWW])

                const __m256i shuf = _mm256_setr_epi8(
                       2,  1,  0,
                       6,  5,  4,
                      10,  9,  8,
                      14, 13, 12,
                      char(0xff), char(0xff), char(0xff), char(0xff),
                       2,  1,  0,
                       6,  5,  4,
                      10,  9,  8,
                      14, 13, 12,
                      char(0xff), char(0xff), char(0xff), char(0xff)
                );

                const __m256i shuffled = _mm256_shuffle_epi8(merged, shuf);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), _mm256_extracti128_si256(shuffled, 0));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 12), _mm256_extracti128_si256(shuffled, 1));

                out += 24;
            }
        }


#if defined(HAVE_BMI2_INSTRUCTIONS)
        __m256i bswap_si256(const __m256i in) {
            return _mm256_shuffle_epi8(in, _mm256_setr_epi8(
                         3,  2,  1,  0,
                         7,  6,  5,  4,
                        11, 10,  9,  8,
                        15, 14, 13, 12,
                         3,  2,  1,  0,
                         7,  6,  5,  4,
                        11, 10,  9,  8,
                        15, 14, 13, 12
                   ));
        }

        uint64_t pack_bytes(uint64_t v) {

                const uint64_t p  = _pext_u64(v, 0x3f3f3f3f3f3f3f3f);

                const uint64_t b0 = p & 0x0000ff0000ff;
                const uint64_t b1 = p & 0x00ff0000ff00;
                const uint64_t b2 = p & 0xff0000ff0000;

                return (b0 << 16) | b1 | (b2 >> 16);
        }

        template <typename FN>
        void decode_bmi2(FN lookup, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 32 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 32) {

                __m256i in = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));
                __m256i values;

                try {
                    values = bswap_si256(lookup(in));
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                // input:  packed_dword([00dddddd|00cccccc|00bbbbbb|00aaaaaa] x 4)
                // merged: packed_dword([00000000|ddddddcc|ccccbbbb|bbaaaaaa] x 4)

                const __m128i lane0 = _mm256_extracti128_si256(values, 0);
                const __m128i lane1 = _mm256_extracti128_si256(values, 1);

                const uint64_t t0 = pack_bytes(_mm_extract_epi64(lane0, 0));
                const uint64_t t1 = pack_bytes(_mm_extract_epi64(lane0, 1));
                const uint64_t t2 = pack_bytes(_mm_extract_epi64(lane1, 0));
                const uint64_t t3 = pack_bytes(_mm_extract_epi64(lane1, 1));

#if 0 // naive store
                *reinterpret_cast<uint64_t*>(out + 0*0) = t0;
                *reinterpret_cast<uint64_t*>(out + 1*6) = t1;
                *reinterpret_cast<uint64_t*>(out + 2*6) = t2;
                *reinterpret_cast<uint64_t*>(out + 3*6) = t3;
#else
                *reinterpret_cast<uint64_t*>(out + 0*8) = (t1 << (6*8)) | t0;
                *reinterpret_cast<uint64_t*>(out + 1*8) = (t2 << (4*8)) | (t1 >> (2*8));
                *reinterpret_cast<uint64_t*>(out + 2*8) = (t3 << (2*8)) | (t2 >> (4*8));
#endif
                out += 24;
            }
        }
#endif // defined(HAVE_BMI2_INSTRUCTIONS)

    } // namespace avx2_templates

} // namespace base64
