namespace base64 {

    namespace sse {

        template <typename FN_LOOKUP, typename FN_PACK>
        void decode(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 16 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 16) {

                __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));
                __m128i values;

                try {
                    values = lookup(in);
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                // input:  packed_dword([00dddddd|00cccccc|00bbbbbb|00aaaaaa] x 4)
                // merged: packed_dword([00000000|ddddddcc|ccccbbbb|bbaaaaaa] x 4)

                const __m128i merged = pack(values);

                // merged = packed_byte([0XXX|0YYY|0ZZZ|0WWW])

                const __m128i shuf = _mm_setr_epi8(
                        2,  1,  0,
                        6,  5,  4,
                       10,  9,  8,
                       14, 13, 12,
                      char(0xff), char(0xff), char(0xff), char(0xff)
                );

                // lower 12 bytes contains the result
                const __m128i shuffled = _mm_shuffle_epi8(merged, shuf);

#if 0
                // Note: on Core i5 maskmove is slower than bare write
                const __m128i mask = _mm_setr_epi8(
                      char(0xff), char(0xff), char(0xff), char(0xff),
                      char(0xff), char(0xff), char(0xff), char(0xff),
                      char(0xff), char(0xff), char(0xff), char(0xff),
                      char(0x00), char(0x00), char(0x00), char(0x00)
                );
                _mm_maskmoveu_si128(shuffled, mask, reinterpret_cast<char*>(out));
#else
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), shuffled);
#endif
                out += 12;
            }
        }


#if defined(HAVE_BMI2_INSTRUCTIONS)
        __m128i bswap_si128(const __m128i in) {
            return _mm_shuffle_epi8(in, _mm_setr_epi8(
                         3,  2,  1,  0,
                         7,  6,  5,  4,
                        11, 10,  9,  8,
                        15, 14, 13, 12
                   ));
        }

        template <typename FN>
        void decode_bmi2(FN lookup, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 16 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 16) {

                __m128i in = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));
                __m128i values;

                try {
                    values = bswap_si128(lookup(in));
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                // input:  packed_dword([00dddddd|00cccccc|00bbbbbb|00aaaaaa] x 4)
                // merged: packed_dword([00000000|ddddddcc|ccccbbbb|bbaaaaaa] x 4)

                const uint64_t lo = _mm_extract_epi64(values, 0);
                const uint64_t hi = _mm_extract_epi64(values, 1);

                uint64_t t0 = _pext_u64(lo, 0x3f3f3f3f3f3f3f3f);
                uint64_t t1 = _pext_u64(hi, 0x3f3f3f3f3f3f3f3f);

                uint64_t b0, b1, b2;

                b0 = t0 & 0x0000ff0000ff;
                b1 = t0 & 0x00ff0000ff00;
                b2 = t0 & 0xff0000ff0000;
                t0 = (b0 << 16) | b1 | (b2 >> 16);

                b0 = t1 & 0x0000ff0000ff;
                b1 = t1 & 0x00ff0000ff00;
                b2 = t1 & 0xff0000ff0000;
                t1 = (b0 << 16) | b1 | (b2 >> 16);

                *reinterpret_cast<uint64_t*>(out + 0)  = t0;
                *reinterpret_cast<uint64_t*>(out + 6)  = t1;
                out += 12;
            }
        }
#endif // defined(HAVE_BMI2_INSTRUCTIONS)

        // The algorithm by aqrit. It uses a clever hashing of input bytes
        void decode_aqrit(const uint8_t* input, size_t size, uint8_t* output) {

            const __m128i delta_asso = _mm_setr_epi8(
                    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F
            );
            const __m128i delta_values = _mm_setr_epi8(
                    int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13),
                    int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
                    int8_t(0xB9), int8_t(0x00), int8_t(0x10), int8_t(0xC3),
                    int8_t(0xBF), int8_t(0xBF), int8_t(0xB9), int8_t(0xB9)
            );
            const __m128i check_asso = _mm_setr_epi8(
                    0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                    0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F
            );
            const __m128i check_values = _mm_setr_epi8(
                    int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80),
                    int8_t(0xCF), int8_t(0xBF), int8_t(0xD5), int8_t(0xA6),
                    int8_t(0xB5), int8_t(0x86), int8_t(0xD1), int8_t(0x80),
                    int8_t(0xB1), int8_t(0x80), int8_t(0x91), int8_t(0x80)
            );

            for (size_t i=0; i < size; i += 16) {

                const __m128i src     = _mm_loadu_si128((__m128i*)(input + i));
                const __m128i shifted = _mm_srli_epi32(src, 3);

                const __m128i delta_hash = _mm_avg_epu8(_mm_shuffle_epi8(delta_asso, src), shifted);
                const __m128i check_hash = _mm_avg_epu8(_mm_shuffle_epi8(check_asso, src), shifted);

                const __m128i out = _mm_adds_epi8(_mm_shuffle_epi8(delta_values, delta_hash), src);
                const __m128i chk = _mm_adds_epi8(_mm_shuffle_epi8(check_values, check_hash), src);

                const int mask = _mm_movemask_epi8(chk);
                if (mask != 0) {
                    const int pos = __builtin_ctz(mask);
                    throw invalid_input(i + pos, input[i + pos]);
                }

                const __m128i pack_shuffle = _mm_setr_epi8(
                    2,  1,  0,  6,  5,  4, 10,  9,
                    8, 14, 13, 12, -1, -1, -1, -1
                );

                const __m128i t0 = _mm_maddubs_epi16(out, _mm_set1_epi32(0x01400140));
                const __m128i t1 = _mm_madd_epi16(t0, _mm_set1_epi32(0x00011000));
                const __m128i t2 = _mm_shuffle_epi8(t1, pack_shuffle);

                _mm_storeu_si128((__m128i*)output, t2);
                output += 12;
            }
        }

    } // namespace sse

} // namespace base64


