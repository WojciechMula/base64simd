// a translation of lookup.sse.cpp
#include <arm_neon.h>

namespace base64 {

    namespace neon {

    #define packed_byte(x) vdupq_n_u8(x)

        uint8x16_t lookup_naive(const uint8x16_t input) {
            
            const uint8x16_t less_26  = vcltq_u8(input, packed_byte(26));
            const uint8x16_t less_52  = vcltq_u8(input, packed_byte(52));
            const uint8x16_t less_62  = vcltq_u8(input, packed_byte(62));
            const uint8x16_t equal_62 = vceqq_u8(input, packed_byte(62));
            const uint8x16_t equal_63 = vceqq_u8(input, packed_byte(63));

            const uint8x16_t range_AZ = vandq_u8(packed_byte('A'), less_26);
            const uint8x16_t range_az = vandq_u8(packed_byte('a' - 26), vbicq_u8(less_52, less_26));
            const uint8x16_t range_09 = vandq_u8(packed_byte('0' - 52), vbicq_u8(less_62, less_52));
            const uint8x16_t char_plus  = vandq_u8(packed_byte('+' - 62), equal_62);
            const uint8x16_t char_slash = vandq_u8(packed_byte('/' - 63), equal_63);

            uint8x16_t shift;
            shift = vorrq_u8(range_AZ, range_az);
            shift = vorrq_u8(shift, range_09);
            shift = vorrq_u8(shift, char_plus);
            shift = vorrq_u8(shift, char_slash);

            return vaddq_u8(shift, input);
        }

        uint8x16_t lookup_version2(const uint8x16_t input) {

            uint8x16_t result = packed_byte(65);

            const uint8x16_t ge_26 = vcgtq_u8(input, packed_byte(25));
            const uint8x16_t ge_52 = vcgtq_u8(input, packed_byte(51));
            const uint8x16_t eq_62 = vceqq_u8(input, packed_byte(62));
            const uint8x16_t eq_63 = vceqq_u8(input, packed_byte(63));

            result = vaddq_u8(result, vandq_u8(ge_26, packed_byte(  6)));
            result = vsubq_u8(result, vandq_u8(ge_52, packed_byte( 75)));
            result = vaddq_u8(result, vandq_u8(eq_62, packed_byte(241)));
            result = vsubq_u8(result, vandq_u8(eq_63, packed_byte( 12)));

            result = vaddq_u8(result, input);

            return result;
        }

        uint8x16_t lookup_pshufb_improved(const uint8x16_t input) {

            // reduce  0..51 -> 0
            //        52..61 -> 1 .. 10
            //            62 -> 11
            //            63 -> 12
            uint8x16_t result = vqsubq_u8(input, packed_byte(51));

            // distinguish between ranges 0..25 and 26..51:
            //         0 .. 25 -> remains 0
            //        26 .. 51 -> becomes 13
            const uint8x16_t less = vcgtq_u8(packed_byte(26), input);
            result = vorrq_u8(result, vandq_u8(less, packed_byte(13)));

            const uint8x8_t shift_LUT_lo = {
                uint8_t('a' - 26), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52),
                uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52)};

            const uint8x8_t shift_LUT_hi = {
                uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('0' - 52), uint8_t('+' - 62),
                uint8_t('/' - 63), 'A', 0, 0};

            const uint8x8x2_t shift_LUT = {shift_LUT_lo, shift_LUT_hi};

            // read shift
            result = vcombine_u8(
                      vtbl2_u8(shift_LUT, vget_low_u8(result)),
                      vtbl2_u8(shift_LUT, vget_high_u8(result)));

            return vaddq_u8(result, input);
        }

    #undef packed_byte

    } // namespace neon

} // namespace base64

