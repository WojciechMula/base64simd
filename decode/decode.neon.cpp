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

            uint8x8x3_t result;

            for (size_t i=0; i < size; i += 8*4) {

                const uint8x8x4_t in = vld4_u8(input + i);

                try {
                    field_a = lookup(in.val[0]);
                    field_b = lookup(in.val[1]);
                    field_c = lookup(in.val[2]);
                    field_d = lookup(in.val[3]);
                } catch (invalid_input& e) {

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


