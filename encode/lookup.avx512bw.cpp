namespace base64 {

    namespace avx512bw {

        // direct translations from lookup.sse.cpp

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


        __m512i lookup_pshufb_improved(const __m512i input) {

            // reduce  0..51 -> 0
            //        52..61 -> 1 .. 10
            //            62 -> 11
            //            63 -> 12
            __m512i result = _mm512_subs_epu8(input, packed_byte(51));

            // distinguish between ranges 0..25 and 26..51:
            //         0 .. 25 -> remains 0
            //        26 .. 51 -> becomes 13
            const __mmask64 less = _mm512_cmpgt_epi8_mask(packed_byte(26), input);
            result = _mm512_mask_mov_epi8(result, less, packed_byte(13));

            /* the SSE lookup
                const __m128i shift_LUT = _mm_setr_epi8(
                    'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                    '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62,
                    '/' - 63, 'A', 0, 0
                );
                which is:
                    0x47, 0xfc, 0xfc, 0xfc,
                    0xfc, 0xfc, 0xfc, 0xfc,
                    0xfc, 0xfc, 0xfc, 0xed,
                    0xf0, 0x41, 0x00, 0x00

                Note that the order of above list is reserved (due to _mm_setr_epi8),
                so the invocation _mm512_set4_epi32 looks... odd.
            */
            const __m512i shift_LUT = _mm512_set4_epi32(
                0x000041f0,
                0xedfcfcfc,
                0xfcfcfcfc,
                0xfcfcfc47
            );

            // read shift
            result = _mm512_shuffle_epi8(shift_LUT, result);

            return _mm512_add_epi8(result, input);
        }

#undef packed_dword
#undef packed_byte

    } // namespace avx512bw

} // namespace base64
