namespace base64 {

    namespace sse {

#define packed_dword(x) _mm_set1_epi32(x)
#define packed_byte(x) _mm_set1_epi8(char(x))

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

                // bytes from groups A, B and C are needed in separate 32-bit lanes
                // in = [DDDD|CCCC|BBBB|AAAA]
                //
                //      an input triplet has layout
                //      [????????|ccdddddd|bbbbcccc|aaaaaabb]
                //        byte 3   byte 2   byte 1   byte 0    -- byte 3 comes from the next triplet
                //
                //      shuffling changes the order of bytes: 1, 0, 2, 1
                //      [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
                //           ^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^
                //                  processed bits
                in = _mm_shuffle_epi8(in, shuf);

                // unpacking

                // t0    = [0000cccc|cc000000|aaaaaa00|00000000]
                const __m128i t0 = _mm_and_si128(in, _mm_set1_epi32(0x0fc0fc00));
                // t1    = [00000000|00cccccc|00000000|00aaaaaa]
                //          (c * (1 << 10), a * (1 << 6)) >> 16 (note: an unsigned multiplication)
                const __m128i t1 = _mm_mulhi_epu16(t0, _mm_set1_epi32(0x04000040));

                // t2    = [00000000|00dddddd|000000bb|bbbb0000]
                const __m128i t2 = _mm_and_si128(in, _mm_set1_epi32(0x003f03f0));
                // t3    = [00dddddd|00000000|00bbbbbb|00000000](
                //          (d * (1 << 8), b * (1 << 4))
                const __m128i t3 = _mm_mullo_epi16(t2, _mm_set1_epi32(0x01000010));

                // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
                const __m128i indices = _mm_or_si128(t1, t3);

                const auto result = lookup(indices);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                out += 16;
            }
        }


        template <typename LOOKUP_FN>
        void encode_unrolled(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const __m128i shuf = _mm_set_epi8(
                10, 11, 9, 10,
                 7,  8, 6,  7,
                 4,  5, 3,  4,
                 1,  2, 0,  1
            );

            for (size_t i = 0; i < bytes; i += 4*3 * 4) {

                // unrolled improved version

                __m128i in0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 0));
                __m128i in1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 1));
                __m128i in2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 2));
                __m128i in3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 3));

                in0 = _mm_shuffle_epi8(in0, shuf);
                in1 = _mm_shuffle_epi8(in1, shuf);
                in2 = _mm_shuffle_epi8(in2, shuf);
                in3 = _mm_shuffle_epi8(in3, shuf);

                const __m128i t0_0 = _mm_and_si128(in0, _mm_set1_epi32(0x0fc0fc00));
                const __m128i t0_1 = _mm_and_si128(in1, _mm_set1_epi32(0x0fc0fc00));
                const __m128i t0_2 = _mm_and_si128(in2, _mm_set1_epi32(0x0fc0fc00));
                const __m128i t0_3 = _mm_and_si128(in3, _mm_set1_epi32(0x0fc0fc00));

                const __m128i t1_0 = _mm_mulhi_epu16(t0_0, _mm_set1_epi32(0x04000040));
                const __m128i t1_1 = _mm_mulhi_epu16(t0_1, _mm_set1_epi32(0x04000040));
                const __m128i t1_2 = _mm_mulhi_epu16(t0_2, _mm_set1_epi32(0x04000040));
                const __m128i t1_3 = _mm_mulhi_epu16(t0_3, _mm_set1_epi32(0x04000040));
                
                const __m128i t2_0 = _mm_and_si128(in0, _mm_set1_epi32(0x003f03f0));
                const __m128i t2_1 = _mm_and_si128(in1, _mm_set1_epi32(0x003f03f0));
                const __m128i t2_2 = _mm_and_si128(in2, _mm_set1_epi32(0x003f03f0));
                const __m128i t2_3 = _mm_and_si128(in3, _mm_set1_epi32(0x003f03f0));
                
                const __m128i t3_0 = _mm_mullo_epi16(t2_0, _mm_set1_epi32(0x01000010));
                const __m128i t3_1 = _mm_mullo_epi16(t2_1, _mm_set1_epi32(0x01000010));
                const __m128i t3_2 = _mm_mullo_epi16(t2_2, _mm_set1_epi32(0x01000010));
                const __m128i t3_3 = _mm_mullo_epi16(t2_3, _mm_set1_epi32(0x01000010));

                const __m128i input0 = _mm_or_si128(t1_0, t3_0);
                const __m128i input1 = _mm_or_si128(t1_1, t3_1);
                const __m128i input2 = _mm_or_si128(t1_2, t3_2);
                const __m128i input3 = _mm_or_si128(t1_3, t3_3);

                {
                    const auto result = lookup(input0);

                    _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                    out += 16;
                }

                {
                    const auto result = lookup(input1);

                    _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                    out += 16;
                }

                {
                    const auto result = lookup(input2);

                    _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                    out += 16;
                }

                {
                    const auto result = lookup(input3);

                    _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                    out += 16;
                }
            }
        }


        void encode_full_unrolled(const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const __m128i shuf = _mm_set_epi8(
                10, 11, 9, 10,
                 7,  8, 6,  7,
                 4,  5, 3,  4,
                 1,  2, 0,  1
            );

            for (size_t i = 0; i < bytes; i += 4*3 * 4) {
                
                // the same code as in encode_unrolled (no, no macros!)
                __m128i in0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 0));
                __m128i in1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 1));
                __m128i in2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 2));
                __m128i in3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4*3 * 3));

                in0 = _mm_shuffle_epi8(in0, shuf);
                in1 = _mm_shuffle_epi8(in1, shuf);
                in2 = _mm_shuffle_epi8(in2, shuf);
                in3 = _mm_shuffle_epi8(in3, shuf);

                const __m128i t0_0 = _mm_and_si128(in0, _mm_set1_epi32(0x0fc0fc00));
                const __m128i t0_1 = _mm_and_si128(in1, _mm_set1_epi32(0x0fc0fc00));
                const __m128i t0_2 = _mm_and_si128(in2, _mm_set1_epi32(0x0fc0fc00));
                const __m128i t0_3 = _mm_and_si128(in3, _mm_set1_epi32(0x0fc0fc00));

                const __m128i t1_0 = _mm_mulhi_epu16(t0_0, _mm_set1_epi32(0x04000040));
                const __m128i t1_1 = _mm_mulhi_epu16(t0_1, _mm_set1_epi32(0x04000040));
                const __m128i t1_2 = _mm_mulhi_epu16(t0_2, _mm_set1_epi32(0x04000040));
                const __m128i t1_3 = _mm_mulhi_epu16(t0_3, _mm_set1_epi32(0x04000040));
                
                const __m128i t2_0 = _mm_and_si128(in0, _mm_set1_epi32(0x003f03f0));
                const __m128i t2_1 = _mm_and_si128(in1, _mm_set1_epi32(0x003f03f0));
                const __m128i t2_2 = _mm_and_si128(in2, _mm_set1_epi32(0x003f03f0));
                const __m128i t2_3 = _mm_and_si128(in3, _mm_set1_epi32(0x003f03f0));
                
                const __m128i t3_0 = _mm_mullo_epi16(t2_0, _mm_set1_epi32(0x01000010));
                const __m128i t3_1 = _mm_mullo_epi16(t2_1, _mm_set1_epi32(0x01000010));
                const __m128i t3_2 = _mm_mullo_epi16(t2_2, _mm_set1_epi32(0x01000010));
                const __m128i t3_3 = _mm_mullo_epi16(t2_3, _mm_set1_epi32(0x01000010));

                const __m128i input0 = _mm_or_si128(t1_0, t3_0);
                const __m128i input1 = _mm_or_si128(t1_1, t3_1);
                const __m128i input2 = _mm_or_si128(t1_2, t3_2);
                const __m128i input3 = _mm_or_si128(t1_3, t3_3);

                // unrolled lookup_version2 from lookup.sse.cpp
                __m128i result_0 = packed_byte(65);
                __m128i result_1 = packed_byte(65);
                __m128i result_2 = packed_byte(65);
                __m128i result_3 = packed_byte(65);

                const __m128i ge_26_0 = _mm_cmpgt_epi8(input0, packed_byte(25));
                result_0 = _mm_add_epi8(result_0, _mm_and_si128(ge_26_0, packed_byte(  6)));
                const __m128i ge_26_1 = _mm_cmpgt_epi8(input1, packed_byte(25));
                result_1 = _mm_add_epi8(result_1, _mm_and_si128(ge_26_1, packed_byte(  6)));
                const __m128i ge_26_2 = _mm_cmpgt_epi8(input2, packed_byte(25));
                result_2 = _mm_add_epi8(result_2, _mm_and_si128(ge_26_2, packed_byte(  6)));
                const __m128i ge_26_3 = _mm_cmpgt_epi8(input3, packed_byte(25));
                result_3 = _mm_add_epi8(result_3, _mm_and_si128(ge_26_3, packed_byte(  6)));

                const __m128i ge_52_0 = _mm_cmpgt_epi8(input0, packed_byte(51));
                result_0 = _mm_sub_epi8(result_0, _mm_and_si128(ge_52_0, packed_byte( 75)));
                const __m128i ge_52_1 = _mm_cmpgt_epi8(input1, packed_byte(51));
                result_1 = _mm_sub_epi8(result_1, _mm_and_si128(ge_52_1, packed_byte( 75)));
                const __m128i ge_52_2 = _mm_cmpgt_epi8(input2, packed_byte(51));
                result_2 = _mm_sub_epi8(result_2, _mm_and_si128(ge_52_2, packed_byte( 75)));
                const __m128i ge_52_3 = _mm_cmpgt_epi8(input3, packed_byte(51));
                result_3 = _mm_sub_epi8(result_3, _mm_and_si128(ge_52_3, packed_byte( 75)));

                const __m128i eq_62_0 = _mm_cmpeq_epi8(input0, packed_byte(62));
                result_0 = _mm_add_epi8(result_0, _mm_and_si128(eq_62_0, packed_byte(241)));
                const __m128i eq_62_1 = _mm_cmpeq_epi8(input1, packed_byte(62));
                result_1 = _mm_add_epi8(result_1, _mm_and_si128(eq_62_1, packed_byte(241)));
                const __m128i eq_62_2 = _mm_cmpeq_epi8(input2, packed_byte(62));
                result_2 = _mm_add_epi8(result_2, _mm_and_si128(eq_62_2, packed_byte(241)));
                const __m128i eq_62_3 = _mm_cmpeq_epi8(input3, packed_byte(62));
                result_3 = _mm_add_epi8(result_3, _mm_and_si128(eq_62_3, packed_byte(241)));

                const __m128i eq_63_0 = _mm_cmpeq_epi8(input0, packed_byte(63));
                result_0 = _mm_sub_epi8(result_0, _mm_and_si128(eq_63_0, packed_byte( 12)));
                const __m128i eq_63_1 = _mm_cmpeq_epi8(input1, packed_byte(63));
                result_1 = _mm_sub_epi8(result_1, _mm_and_si128(eq_63_1, packed_byte( 12)));
                const __m128i eq_63_2 = _mm_cmpeq_epi8(input2, packed_byte(63));
                result_2 = _mm_sub_epi8(result_2, _mm_and_si128(eq_63_2, packed_byte( 12)));
                const __m128i eq_63_3 = _mm_cmpeq_epi8(input3, packed_byte(63));
                result_3 = _mm_sub_epi8(result_3, _mm_and_si128(eq_63_3, packed_byte( 12)));

                result_0 = _mm_add_epi8(result_0, input0);
                result_1 = _mm_add_epi8(result_1, input1);
                result_2 = _mm_add_epi8(result_2, input2);
                result_3 = _mm_add_epi8(result_3, input3);

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result_0);
                out += 16;
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result_1);
                out += 16;
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result_2);
                out += 16;
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result_3);
                out += 16;
            }
        }


#if defined(HAVE_BMI2_INSTRUCTIONS)
        __m128i _mm_bswap_epi32(const __m128i v) {
            return _mm_shuffle_epi8(v, _mm_setr_epi8(
                 7,  6,  5,  4,
                 3,  2,  1,  0,
                15, 14, 13, 12,
                11, 10,  9,  8
            ));
        }

        template <typename LOOKUP_FN>
        void encode_bmi2(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            uint64_t lo = *reinterpret_cast<const uint64_t*>(input + 0);
            uint64_t hi = *reinterpret_cast<const uint64_t*>(input + 0 + 6);
            uint64_t t0 = __builtin_bswap64(lo) >> 16;
            uint64_t t1 = __builtin_bswap64(hi) >> 16;
            uint64_t expanded_lo = _pdep_u64(t0, 0x3f3f3f3f3f3f3f3flu);
            uint64_t expanded_hi = _pdep_u64(t1, 0x3f3f3f3f3f3f3f3flu);

            for (size_t i = 0; i < bytes; i += 2*6) {
#if 1
                __m128i indices;

                indices = _mm_insert_epi64(indices, expanded_lo, 0);
                indices = _mm_insert_epi64(indices, expanded_hi, 1);
#else
                const __m128i indices = _mm_set_epi64x(expanded_hi, expanded_lo);
#endif
                lo = *reinterpret_cast<const uint64_t*>(input + i + 12);
                hi = *reinterpret_cast<const uint64_t*>(input + i + 12 + 6);
                uint64_t t0 = __builtin_bswap64(lo) >> 16;
                uint64_t t1 = __builtin_bswap64(hi) >> 16;
                expanded_lo = _pdep_u64(t0, 0x3f3f3f3f3f3f3f3flu);
                expanded_hi = _pdep_u64(t1, 0x3f3f3f3f3f3f3f3flu);

                const auto result = lookup(_mm_bswap_epi32(indices));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(out), result);
                out += 16;
            }
        }
#endif


    #undef packed_dword
    #undef packed_byte

    } // namespace sse

} // namespace base64
