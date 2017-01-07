#include <arm_neon.h>

namespace base64 {

    namespace neon {

#define packed_byte(x) vdupq_n_u8(char(x))

        template <typename LOOKUP_FN>
        void encode(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 16*3) {
                //      [ccdddddd|bbbbcccc|aaaaaabb]
                //        byte 2   byte 1   byte 0
                
                const uint8x16x3_t in = vld3q_u8(input + i);

                const uint8x16_t field_a = vshrq_n_u8(in.val[0], 2);
                const uint8x16_t field_b = vandq_u8(
                                            vorrq_u8(vshrq_n_u8(in.val[1], 4), vshlq_n_u8(in.val[0], 4)),
                                            packed_byte(0x3f));
                const uint8x16_t field_c = vandq_u8(
                                            vorrq_u8(vshrq_n_u8(in.val[2], 6), vshlq_n_u8(in.val[1], 2)),
                                            packed_byte(0x3f));
                const uint8x16_t field_d = vandq_u8(in.val[2], packed_byte(0x3f));

                uint8x16x4_t result;
                result.val[0] = lookup(field_a);
                result.val[1] = lookup(field_b);
                result.val[2] = lookup(field_c);
                result.val[3] = lookup(field_d);

                vst4q_u8(out, result);
                out += 64;
            }
        }

#undef packed_byte

    } // namespace neon

} // namespace base64
