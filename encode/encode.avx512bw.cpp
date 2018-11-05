namespace base64 {

    namespace avx512bw {

        template <typename LOOKUP_FN, typename UNPACK_FN>
        void encode(LOOKUP_FN lookup, UNPACK_FN unpack, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 4 * 12) {
                // load input 4 x 12 bytes, layout of 12-byte subarray:
                // tmp1 = [D2 D1 D0 C2|C1 C0 B2 B1|B0 A2 A1 A0] x 4, plus unused 16 bytes
                //           dword 2     dword 1     dword 0
                const __m512i tmp1 = _mm512_loadu_si512(input + i);

                // place each 12-byte subarray in seprate 128-bit lane
                // tmp2 = [?? ?? ?? ??|D2 D1 D0 C2|C1 C0 B2 B1|B0 A2 A1 A0] x 4
                //           ignored
                const __m512i tmp2 = _mm512_permutexvar_epi32(
                    _mm512_set_epi32(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0),
                    tmp1
                );

                // reshuffle bytes within 128-bit lanes to format required by
                // AVX512BW unpack procedure
                // tmp2 = [D1 D2 D0 D1|C1 C2 C0 C1|B1 B2 B0 B1|A1 A2 A0 A1] x 4
                //         10 11 9  10 7  8  6  7  4  5  3  4  1  2  0  1
                const __m512i tmp3 = _mm512_shuffle_epi8(
                    tmp2,
                    _mm512_set4_epi32(0x0a0b090a, 0x07080607, 0x04050304, 0x01020001)
                );

                const __m512i indices = unpack(tmp3);

                // do lookup
                const __m512i result = lookup(indices);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), result);

                out += 64;
            }
        }


        // the above template unrolled
        template <typename LOOKUP_FN, typename UNPACK_FN>
        void encode_unrolled2(LOOKUP_FN lookup, UNPACK_FN unpack, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;
            const __m512i permute_lookup = _mm512_set_epi32(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0);
            const __m512i shuffle_lookup = _mm512_set4_epi32(0x0a0b090a, 0x07080607, 0x04050304, 0x01020001);

            for (size_t i = 0; i < bytes; i += 4 * 12 * 2) {
                const __m512i a0 = _mm512_loadu_si512(input + i);
                const __m512i b0 = _mm512_loadu_si512(input + i + 4 * 12);

                const __m512i a1 = _mm512_permutexvar_epi32(permute_lookup, a0);
                const __m512i b1 = _mm512_permutexvar_epi32(permute_lookup, b0);

                const __m512i a2 = _mm512_shuffle_epi8(a1, shuffle_lookup);
                const __m512i b2 = _mm512_shuffle_epi8(b1, shuffle_lookup);

                const __m512i indices_a = unpack(a2);
                const __m512i indices_b = unpack(b2);

                // do lookup
                const __m512i result_a = lookup(indices_a);
                const __m512i result_b = lookup(indices_b);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out),      result_a);
                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out + 64), result_b);

                out += 2 * 64;
            }
        }

    } // namespace avx512bw

} // namespace base64
