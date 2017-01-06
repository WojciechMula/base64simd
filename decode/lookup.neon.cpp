
namespace base64 {

    namespace neon {

#define packed_byte(x) vdup_n_u8(x)
#define has_zero_byte(v) (((v) - 0x01010101UL) & ~(v) & 0x80808080UL)

        uint8x8_t lookup_byte_blend(const uint8x8_t input) {

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

            // XXX: this is very slow, need to figure out how to speed it up
            const uint32_t lo = vget_lane_u32(vreinterpret_u32_u8(shift), 0);
            const uint32_t hi = vget_lane_u32(vreinterpret_u32_u8(shift), 1);
            if (has_zero_byte(lo) || has_zero_byte(hi)) {
                throw invalid_input(0, 0);
            }

            return vadd_u8(input, shift);
        }

#undef packed_byte
#undef has_zero_byte

    } // namespace sse

} // namespace base64

