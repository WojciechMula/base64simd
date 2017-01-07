
namespace base64 {

    namespace neon {

#define packed_byte(x) vdup_n_u8(x)

        uint8x8_t FORCE_INLINE lookup_byte_blend(const uint8x8_t input, uint8x8_t& error) {

            uint8x8_t shift;

            // shift for range 'A' - 'Z'
            const uint8x8_t ge_A = vcgt_u8(input, packed_byte('A' - 1));
            const uint8x8_t le_Z = vclt_u8(input, packed_byte('Z' + 1));
            shift = vand_u8(packed_byte(-65), vand_u8(ge_A, le_Z));

            // shift for range 'a' - 'z'
            const uint8x8_t ge_a = vcgt_u8(input, packed_byte('a' - 1));
            const uint8x8_t le_z = vclt_u8(input, packed_byte('z' + 1));
            shift = vbsl_u8(vand_u8(ge_a, le_z), packed_byte(-71), shift);

            // shift for range '0' - '9'
            const uint8x8_t ge_0 = vcgt_u8(input, packed_byte('0' - 1));
            const uint8x8_t le_9 = vclt_u8(input, packed_byte('9' + 1));
            shift = vbsl_u8(vand_u8(ge_0, le_9), packed_byte(4), shift);

            // shift for character '+'
            const uint8x8_t eq_plus = vceq_u8(input, packed_byte('+'));
            shift = vbsl_u8(eq_plus, packed_byte(19), shift);

            // shift for character '/'
            const uint8x8_t eq_slash = vceq_u8(input, packed_byte('/'));
            shift = vbsl_u8(eq_slash, packed_byte(16), shift);

            error = vceq_u8(shift, packed_byte(0));

            return vadd_u8(input, shift);
        }


        uint8x8_t FORCE_INLINE lookup_pshufb_bitmask(const uint8x8_t input, uint8x8_t& error) {

            const uint8x8_t higher_nibble = vshr_n_u8(input, 4);
            const uint8x8_t lower_nibble  = vand_u8(input, packed_byte(0x0f));

            const uint8x8x2_t shiftLUT = {
                0,   0,  19,   4, uint8_t(-65), uint8_t(-65), uint8_t(-71), uint8_t(-71),
                0,   0,   0,   0,   0,   0,   0,   0};

            const uint8x8x2_t maskLUT  = {
                /* 0        : 0b1010_1000*/ 0xa8,
                /* 1 .. 9   : 0b1111_1000*/ 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8,
                /* 10       : 0b1111_0000*/ 0xf0,
                /* 11       : 0b0101_0100*/ 0x54,
                /* 12 .. 14 : 0b0101_0000*/ 0x50, 0x50, 0x50,
                /* 15       : 0b0101_0100*/ 0x54
            };

            const uint8x8x2_t bitposLUT = {
                0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };

            const uint8x8_t sh     = vtbl2_u8(shiftLUT,  higher_nibble);
            const uint8x8_t eq_2f  = vceq_u8(input, packed_byte(0x2f));
            const uint8x8_t shift  = vbsl_u8(eq_2f, packed_byte(16), sh);

            const uint8x8_t M      = vtbl2_u8(maskLUT,   lower_nibble);
            const uint8x8_t bit    = vtbl2_u8(bitposLUT, higher_nibble);

            error = vceq_u8(vand_u8(M, bit), packed_byte(0));

            const uint8x8_t result = vadd_u8(input, shift);

            return result;
        }

#undef packed_byte

    } // namespace sse

} // namespace base64

