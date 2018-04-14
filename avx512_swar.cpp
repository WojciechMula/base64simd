#if defined(HAVE_AVX2_INSTRUCTIONS)

#define packed_dword(x) _mm512_set1_epi32(x)
#define packed_byte(x) packed_dword((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))

namespace avx512f_swar {

    // returns packed (a[i] >= val) ? 0x7f : 0x00;
    // assertion a[i] < 0x80
    __m512i _mm512_cmpge_mask7bit(const __m512i a, const __m512i val) {

        const __m512i MSB = _mm512_and_si512(_mm512_add_epi32(a, val), packed_byte(0x80));

        // (MSB - (MSB >> 7)
        return _mm512_sub_epi32(MSB, _mm512_srli_epi32(MSB, 7));
    }

    // returns packed (a[i] >= val) ? 0xff : 0x00;
    // assertion a[i] < 0x80
    __m512i _mm512_cmpge_mask8bit(const __m512i a, const __m512i val) {

        const __m512i MSB = _mm512_and_si512(_mm512_add_epi32(a, val), packed_byte(0x80));

        // MSB | (MSB - (MSB >> 7)
        return _mm512_or_si512(_mm512_sub_epi32(MSB, _mm512_srli_epi32(MSB, 7)), MSB);
    }

    // returns packed (a[i] + b[i]) & 0xff
    // assertion a[i] < 0x80, b[i] can be any value
    __m512i _mm512_add_epu8(const __m512i a, const __m512i b) {

        const __m512i b06 = _mm512_and_si512(b, packed_byte(0x7f));
        const __m512i sum = _mm512_add_epi32(a, b06);

        // merge the 7th bit of b with sum
        /*
            MSB  b  sum | result = (MSB & (b_7 ^ sum)) | (~MSB & sum)
            ------------+--------
             0   0   0  |   0
             0   0   1  |   1
             0   1   0  |   0
             0   1   1  |   1
             1   0   0  |   0
             1   0   1  |   1
             1   1   0  |   1
             1   1   1  |   0
         */
        return _mm512_ternarylogic_epi32(packed_byte(0x80), b, sum, 0x6a);
    }

    // returns packed (a[i] >= lo and a[i] <= hi) ? 0x7f : 0x00;
    // assertion a[i] < 0x80
    __m512i _mm512_range_mask_7bit(const __m512i a, const __m512i lo, const __m512i hi) {

        const __m512i L = _mm512_add_epi32(a, lo);
        const __m512i H = _mm512_add_epi32(a, hi);

        // L & ~H & 0x80
        const __m512i MSB = _mm512_ternarylogic_epi32(packed_byte(0x80), H, L, 0x20);

        //  (MSB - (MSB >> 7)
        return _mm512_sub_epi32(MSB, _mm512_srli_epi32(MSB, 7));
    }

    // returns packed (a[i] >= lo and a[i] <= hi) ? 0xff : 0x00;
    // assertion a[i] < 0x80
    __m512i _mm512_range_mask_8bit(const __m512i a, const __m512i lo, const __m512i hi) {

        const __m512i L = _mm512_add_epi32(a, lo);
        const __m512i H = _mm512_add_epi32(a, hi);

        const __m512i MSB = _mm512_ternarylogic_epi32(packed_byte(0x80), H, L, 0x20);

        // MSB | (MSB - (MSB >> 7))
        return _mm512_or_si512(MSB, _mm512_sub_epi32(MSB, _mm512_srli_epi32(MSB, 7)));
    }


    // returns packed (a[i] >= lo and a[i] <= hi) ? 0x04 : 0x00;
    // assertion a[i] < 0x80
    __m512i _mm512_range_3rd_bit(const __m512i a, const __m512i lo, const __m512i hi) {

        const __m512i L = _mm512_add_epi32(a, lo);
        const __m512i H = _mm512_add_epi32(a, hi);

        const __m512i MSB = _mm512_ternarylogic_epi32(packed_byte(0x80), H, L, 0x20);

        return _mm512_srli_epi32(MSB, 5);
    }

} // namespace avx512f_swar

#undef packed_dword
#undef packed_byte

#endif
