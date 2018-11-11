namespace base64 {

    namespace avx512bw {

        __m512i lookup_pshufb_bitmask(const __m512i input) {

            /*
            lower nibble (. is 0)
            ------------------------------------------------------------------------
              0:    .   .   .   4   . -65   . -71   .   .   .   .   .   .   .   .
              1:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              2:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              3:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              4:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              5:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              6:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              7:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              8:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
              9:    .   .   .   4 -65 -65 -71 -71   .   .   .   .   .   .   .   .
             10:    .   .   .   . -65 -65 -71 -71   .   .   .   .   .   .   .   .
             11:    .   .  19   . -65   . -71   .   .   .   .   .   .   .   .   .
             12:    .   .   .   . -65   . -71   .   .   .   .   .   .   .   .   .
             13:    .   .   .   . -65   . -71   .   .   .   .   .   .   .   .   .
             14:    .   .   .   . -65   . -71   .   .   .   .   .   .   .   .   .
             15:    .   .  16   . -65   . -71   .   .   .   .   .   .   .   .   .
                   LSB                                                         MSB
            */

#define packed_byte(b) _mm512_set1_epi8(uint8_t(b))

            const __m512i higher_nibble = _mm512_and_si512(_mm512_srli_epi32(input, 4), packed_byte(0x0f));
            const __m512i lower_nibble  = _mm512_and_si512(input, packed_byte(0x0f));

            const __m512i shiftLUT = _mm512_set4lanes_epi8(
                0,   0,  19,   4, -65, -65, -71, -71,
                0,   0,   0,   0,   0,   0,   0,   0);

            const __m512i maskLUT  = _mm512_set4lanes_epi8(
                /* 0        : 0b1010_1000*/ 0xa8,
                /* 1 .. 9   : 0b1111_1000*/ 0xf8, 0xf8, 0xf8, 0xf8,
                                            0xf8, 0xf8, 0xf8, 0xf8,
                                            0xf8,
                /* 10       : 0b1111_0000*/ 0xf0,
                /* 11       : 0b0101_0100*/ 0x54,
                /* 12 .. 14 : 0b0101_0000*/ 0x50, 0x50, 0x50,
                /* 15       : 0b0101_0100*/ 0x54
            );

            const __m512i bitposLUT = _mm512_set4lanes_epi8(
                0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            );

            const __m512i   sh      = _mm512_shuffle_epi8(shiftLUT,  higher_nibble);
            const __mmask64 eq_2f   = _mm512_cmpeq_epi8_mask(input, packed_byte(0x2f));
            const __m512i   shift   = _mm512_mask_mov_epi8(sh, eq_2f, packed_byte(16));

            const __m512i M         = _mm512_shuffle_epi8(maskLUT,   lower_nibble);
            const __m512i bit       = _mm512_shuffle_epi8(bitposLUT, higher_nibble);

            const uint64_t match    = _mm512_test_epi8_mask(M, bit);

            if (match != uint64_t(-1)) {
                // some characters do not match the valid range
                for (unsigned i=0; i < 64; i++) {
                    if ((match & (uint64_t(1) << i)) == 0) {
                        throw invalid_input(i, 0);
                    }
                }

                throw std::logic_error("method should raise error");
            }
            const __m512i result = _mm512_add_epi8(input, shift);

            return result;
#undef packed_byte
        }

        // The algorithm by aqrit. It uses a clever hashing of input bytes
        __m512i lookup_aqrit(const __m512i input) {

            const __m512i delta_asso = _mm512_set4lanes_epi8(
                    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F
            );
            const __m512i delta_values = _mm512_set4lanes_epi8(
                    int8_t(0x00), int8_t(0x00), int8_t(0x00), int8_t(0x13),
                    int8_t(0x04), int8_t(0xBF), int8_t(0xBF), int8_t(0xB9),
                    int8_t(0xB9), int8_t(0x00), int8_t(0x10), int8_t(0xC3),
                    int8_t(0xBF), int8_t(0xBF), int8_t(0xB9), int8_t(0xB9)
            );
            const __m512i check_asso = _mm512_set4lanes_epi8(
                    0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                    0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F
            );
            const __m512i check_values = _mm512_set4lanes_epi8(
                    int8_t(0x80), int8_t(0x80), int8_t(0x80), int8_t(0x80),
                    int8_t(0xCF), int8_t(0xBF), int8_t(0xD5), int8_t(0xA6),
                    int8_t(0xB5), int8_t(0x86), int8_t(0xD1), int8_t(0x80),
                    int8_t(0xB1), int8_t(0x80), int8_t(0x91), int8_t(0x80)
            );

            const __m512i shifted = _mm512_srli_epi32(input, 3);

            const __m512i delta_hash = _mm512_avg_epu8(_mm512_shuffle_epi8(delta_asso, input), shifted);
            const __m512i check_hash = _mm512_avg_epu8(_mm512_shuffle_epi8(check_asso, input), shifted);

            const __m512i out = _mm512_adds_epi8(_mm512_shuffle_epi8(delta_values, delta_hash), input);
            const __m512i chk = _mm512_adds_epi8(_mm512_shuffle_epi8(check_values, check_hash), input);

            const uint64_t mask = _mm512_movepi8_mask(chk);
            if (mask != 0) {
                const int pos = __builtin_ctzll(mask);
                throw invalid_input(pos, 0);
            }

            return out;
        }


    } // namespace avx512bw

} // namespace base64

