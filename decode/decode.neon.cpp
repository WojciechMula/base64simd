namespace base64 {

    namespace neon {

        template <typename FN_LOOKUP>
        void decode(FN_LOOKUP lookup, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % (16*4) == 0);

            uint8_t* out = output;

            uint8x16_t field_a;
            uint8x16_t field_b;
            uint8x16_t field_c;
            uint8x16_t field_d;

            uint8x16_t error_a;
            uint8x16_t error_b;
            uint8x16_t error_c;
            uint8x16_t error_d;

            union {
                uint8_t  error_mem[8];
                uint32_t error_word[2];
            };

            uint8x16x3_t result;

            for (size_t i=0; i < size; i += 16*4) {

                const uint8x16x4_t in = vld4q_u8(input + i);

                field_a = lookup(in.val[0], error_a);
                field_b = lookup(in.val[1], error_b);
                field_c = lookup(in.val[2], error_c);
                field_d = lookup(in.val[3], error_d);

                /*
                    Due to long trip between the Neon and the core units,
                    it's easier to use temporary memory.

                    A lookup procedure prepares the vector of errors, where non-zero
                    byte means errors. Later we merge them and store in the buffer.
                    Then just one comparison is needed to determine if there were
                    any error.
                */
                const uint8x16_t tmp = vorrq_u8(vorrq_u8(error_a, error_b), vorrq_u8(error_c, error_d));
                const uint8x8_t error = vorr_u8(vget_low_u8(tmp), vget_high_u8(tmp));

                vst1_u8(error_mem, error);
                if (error_word[0] | error_word[1]) {

                    for (size_t j=0; j < 16*4; j++) {
                        if (base64::scalar::lookup[input[i + j]] == 0xff) {
                            throw invalid_input(i + j, input[i + j]);
                        }
                    }

                    assert(false);
                }

                result.val[0] = vorrq_u8(vshrq_n_u8(field_b, 4), vshlq_n_u8(field_a, 2));
                result.val[1] = vorrq_u8(vshrq_n_u8(field_c, 2), vshlq_n_u8(field_b, 4));
                result.val[2] = vorrq_u8(field_d, vshlq_n_u8(field_c, 6));

                vst3q_u8(out, result);
                out += 16*3;
            }
        }

    } // namespace neon

} // namespace base64


