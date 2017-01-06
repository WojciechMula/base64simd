#include <arm_neon.h>

namespace base64 {

    namespace neon {

#define packed_byte(x) vdup_n_u8(char(x))

        template <typename LOOKUP_FN>
        void encode(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 8*3) {
                //      [ccdddddd|bbbbcccc|aaaaaabb]
                //        byte 2   byte 1   byte 0
                
                const uint8x8x3_t in = vld3_u8(input + i);

                const uint8x8_t field_a = vshr_n_u8(in.val[0], 2);
                const uint8x8_t field_b = vand_u8(
                                            vorr_u8(vshr_n_u8(in.val[1], 4), vshl_n_u8(in.val[0], 4)),
                                            packed_byte(0x3f));
                const uint8x8_t field_c = vand_u8(
                                            vorr_u8(vshr_n_u8(in.val[2], 6), vshl_n_u8(in.val[1], 2)),
                                            packed_byte(0x3f));
                const uint8x8_t field_d = vand_u8(in.val[2], packed_byte(0x3f));

                uint8x8x4_t result;
                result.val[0] = lookup(field_a);
                result.val[1] = lookup(field_b);
                result.val[2] = lookup(field_c);
                result.val[3] = lookup(field_d);

                vst4_u8(out, result);
                out += 32;
            }
        }

#undef packed_byte

    } // namespace neon

} // namespace base64
