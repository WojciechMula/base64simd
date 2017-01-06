namespace base64 {

    namespace xop {

        __m128i lookup_pshufb_bitmask(const __m128i input) {

#define packed_byte(b) _mm_set1_epi8(uint8_t(b))

            const __m128i higher_nibble = _mm_shl_epi8(input, packed_byte(-4));
            const __m128i lower_nibble  = _mm_and_si128(input, packed_byte(0x0f));

            const __m128i shiftLUT = _mm_setr_epi8(
                0,   0,  19,   4, -65, -65, -71, -71,
                0,   0,   0,   0,   0,   0,   0,   0);

            const __m128i maskLUT  = _mm_setr_epi8(
                /* 0        */ char(0b10101000),
                /* 1 .. 9   */ char(0b11111000), char(0b11111000), char(0b11111000), char(0b11111000),
                               char(0b11111000), char(0b11111000), char(0b11111000), char(0b11111000),
                               char(0b11111000),
                /* 10       */ char(0b11110000),
                /* 11       */ char(0b01010100),
                /* 12 .. 14 */ char(0b01010000), char(0b01010000), char(0b01010000),
                /* 15       */ char(0b01010100)
            );

            const __m128i sh     = _mm_shuffle_epi8(shiftLUT,  higher_nibble);
            const __m128i eq_2f  = _mm_cmpeq_epi8(input, packed_byte(0x2f));
            const __m128i shift  = _mm_blendv_epi8(sh, packed_byte(16), eq_2f);

            const __m128i M      = _mm_shuffle_epi8(maskLUT, lower_nibble);
            const __m128i bit    = _mm_shl_epi8(packed_byte(1), higher_nibble);

            const __m128i non_match = _mm_cmpeq_epi8(_mm_and_si128(M, bit), _mm_setzero_si128());

            const auto mask = _mm_movemask_epi8(non_match);
            if (mask) {
                // some characters do not match the valid range
                for (unsigned i=0; i < 16; i++) {
                    if (mask & (1 << i)) {
                        throw invalid_input(i, 0);
                    }
                }
            }
            const __m128i result = _mm_add_epi8(input, shift);

            return result;
#undef packed_byte
        }

    } // namespace xop

} // namespace base64

