namespace base64 {

    namespace avx512bw {

#define packed_dword(x) _mm512_set1_epi32(x)
#define packed_byte_aux(x) packed_dword((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))
#define packed_byte(x) packed_byte_aux(uint32_t(uint8_t(x)))

        __m512i lookup_version2(const __m512i input) {

            __m512i result = packed_byte(65);

            const __mmask64 ge_26 = _mm512_cmpgt_epi8_mask(input, packed_byte(25));
            const __mmask64 ge_52 = _mm512_cmpgt_epi8_mask(input, packed_byte(51));
            const __mmask64 eq_62 = _mm512_cmpeq_epi8_mask(input, packed_byte(62));
            const __mmask64 eq_63 = _mm512_cmpeq_epi8_mask(input, packed_byte(63));

            result = _mm512_mask_add_epi8(result, ge_26, result, packed_byte(  6));
            result = _mm512_mask_sub_epi8(result, ge_52, result, packed_byte( 75));
            result = _mm512_mask_add_epi8(result, eq_62, result, packed_byte(241));
            result = _mm512_mask_sub_epi8(result, eq_63, result, packed_byte( 12));

            result = _mm512_add_epi8(result, input);

            return result;
        }

#undef packed_dword
#undef packed_byte

    } // namespace avx512bw

} // namespace base64
