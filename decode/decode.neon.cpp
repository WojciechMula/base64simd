namespace base64 {

    namespace neon {

        template <typename FN_LOOKUP>
        void decode(FN_LOOKUP lookup, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % (8*4) == 0);

            uint8_t* out = output;

            uint8x8_t field_a;
            uint8x8_t field_b;
            uint8x8_t field_c;
            uint8x8_t field_d;

            uint8x8_t error_a;
            uint8x8_t error_b;
            uint8x8_t error_c;
            uint8x8_t error_d;

            union {
                uint8_t  error_mem[8];
                uint32_t error_word[2];
            };

            uint8x8x3_t result;

            for (size_t i=0; i < size; i += 8*4) {

                const uint8x8x4_t in = vld4_u8(input + i);

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
                const uint8x8_t error = vorr_u8(vorr_u8(error_a, error_b), vorr_u8(error_c, error_d));

                vst1_u8(error_mem, error);
                if (error_word[0] | error_word[1]) {

                    for (size_t j=0; j < 8*4; j++) {
                        if (base64::scalar::lookup[input[i + j]] == 0xff) {
                            throw invalid_input(i + j, input[i + j]);
                        }
                    }

                    assert(false);
                }

                result.val[0] = vorr_u8(vshr_n_u8(field_b, 4), vshl_n_u8(field_a, 2));
                result.val[1] = vorr_u8(vshr_n_u8(field_c, 2), vshl_n_u8(field_b, 4));
                result.val[2] = vorr_u8(field_d, vshl_n_u8(field_c, 6));

                vst3_u8(out, result);
                out += 8*3;
            }
        }

    } // namespace neon

} // namespace base64


