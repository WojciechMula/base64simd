// a translation of lookup.sse.cpp
#include <arm_neon.h>

namespace base64 {

    namespace neon {

    #define packed_byte(x) vdup_n_u8(x)

        uint8x8_t lookup_naive(const uint8x8_t input) {
            
            const uint8x8_t less_26  = vclt_u8(input, packed_byte(26));
            const uint8x8_t less_52  = vclt_u8(input, packed_byte(52));
            const uint8x8_t less_62  = vclt_u8(input, packed_byte(62));
            const uint8x8_t equal_62 = vceq_u8(input, packed_byte(62));
            const uint8x8_t equal_63 = vceq_u8(input, packed_byte(63));

            const uint8x8_t range_AZ = vand_u8(packed_byte('A'), less_26);
            const uint8x8_t range_az = vand_u8(packed_byte('a' - 26), vbic_u8(less_52, less_26));
            const uint8x8_t range_09 = vand_u8(packed_byte('0' - 52), vbic_u8(less_62, less_52));
            const uint8x8_t char_plus  = vand_u8(packed_byte('+' - 62), equal_62);
            const uint8x8_t char_slash = vand_u8(packed_byte('/' - 63), equal_63);

            uint8x8_t shift;
            shift = vorr_u8(range_AZ, range_az);
            shift = vorr_u8(shift, range_09);
            shift = vorr_u8(shift, char_plus);
            shift = vorr_u8(shift, char_slash);

            return vadd_u8(shift, input);
        }

        uint8x8_t lookup_version2(const uint8x8_t input) {

            uint8x8_t result = packed_byte(65);

            const uint8x8_t ge_26 = vcgt_u8(input, packed_byte(25));
            const uint8x8_t ge_52 = vcgt_u8(input, packed_byte(51));
            const uint8x8_t eq_62 = vceq_u8(input, packed_byte(62));
            const uint8x8_t eq_63 = vceq_u8(input, packed_byte(63));

            result = vadd_u8(result, vand_u8(ge_26, packed_byte(  6)));
            result = vsub_u8(result, vand_u8(ge_52, packed_byte( 75)));
            result = vadd_u8(result, vand_u8(eq_62, packed_byte(241)));
            result = vsub_u8(result, vand_u8(eq_63, packed_byte( 12)));

            result = vadd_u8(result, input);

            return result;
        }

        uint8x8_t lookup_pshufb_improved(const uint8x8_t input) {

            // reduce  0..51 -> 0
            //        52..61 -> 1 .. 10
            //            62 -> 11
            //            63 -> 12
            uint8x8_t result = vqsub_u8(input, packed_byte(51));

            // distinguish between ranges 0..25 and 26..51:
            //         0 .. 25 -> remains 0
            //        26 .. 51 -> becomes 13
            const uint8x8_t less = vcgt_u8(packed_byte(26), input);
            result = vorr_u8(result, vand_u8(less, packed_byte(13)));

            const uint8x8_t shift_LUT_lo = {
                uint8_t('a' - 26), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52),
                uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52)};

            const uint8x8_t shift_LUT_hi = {
                uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('+' - 62),
                uint8_t('/' - 63), 'A', 0, 0};

            const uint8x8x2_t shift_LUT = {shift_LUT_lo, shift_LUT_hi};

            // read shift
            result = vtbl2_u8(shift_LUT, result);

            return vadd_u8(result, input);
        }

    #undef packed_byte

    } // namespace neon

} // namespace base64

