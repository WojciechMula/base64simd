
namespace base64 {

    namespace swar {

#define packed_dword(x) (uint64_t(x) | uint64_t(x) << 32)
#define packed_word(x) packed_dword(uint32_t(x) | (uint32_t(x) << 16))
#define packed_byte(x) packed_word(uint16_t(x) | (uint16_t(x) << 8))

        uint64_t lookup_incremental_logic(const uint64_t in) {

            uint64_t shift = packed_byte('A');
            uint64_t c0, c1, c2, c3;
            const uint64_t MSB = packed_byte(0x80);

            // shift ^= cmp(i >= 26) & 6;
            c0 = (in + packed_byte(0x80 - 26)) & MSB;
            c0 = c0 - (c0 >> 7);
            c0 = c0 & packed_byte(6);
            
            // shift ^= cmp(i >= 52) & 187;
            c1 = (in + packed_byte(0x80 - 52)) & MSB;
            const uint64_t c1msb = c1;
            c1 = c1 - (c1 >> 7);
            c1 = c1 & packed_byte(187 & 0x7f);

            // shift ^= cmp(i >= 62) & 17;
            c2 = (in + packed_byte(0x80 - 62)) & MSB;
            c2 = c2 - (c2 >> 7);
            c2 = c2 & packed_byte(17);

            // shift ^= cmp(i >= 63) & 29;
            c3 = (in + packed_byte(0x80 - 63)) & MSB;
            c3 = c3 - (c3 >> 7);
            c3 = c3 & packed_byte(29);

            shift = shift ^ c0 ^ c1 ^ c2 ^ c3;

            // produce the result
            return (in + shift) ^ c1msb;
        }

#undef packed_byte
#undef packed_word
#undef packed_dword

    } // namespace swar

} // namespace base64
