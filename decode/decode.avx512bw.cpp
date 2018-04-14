namespace base64 {

    namespace avx512bw {

        template <typename FN_LOOKUP, typename FN_PACK>
        void decode(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 64 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 64) {

                __m512i in = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));
                __m512i values;

                try {
                    values = lookup(in);
                } catch (invalid_input& e) {

                    const auto shift = e.offset;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                const __m512i packed = pack(values);

                //                 |  32 bits  |
                // packed        = [.. D2 D1 D0|.. C2 C1 C0|.. B2 B1 B0|.. A2 A1 A0] x 4 (four 128-bit lanes)
                // index            3           2           1           0

                const __m512i t1 = _mm512_shuffle_epi8(
                    packed,
                    _mm512_set4lanes_epi8(
                         2,  1,  0,
                         6,  5,  4,
                        10,  9,  8,
                        14, 13, 12,
                        -1, -1, -1, -1)
                );

                // shuffle bytes
                const __m512i s6 = _mm512_setr_epi32(
                     0,  1,  2,
                     4,  5,  6,
                     8,  9, 10,
                    12, 13, 14,
                    // unused
                     0,  0,  0, 0);

                const __m512i t2 = _mm512_permutexvar_epi32(s6, t1);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), t2);

                out += 48;
            }
        }

    } // namespace avx512bw

} // namespace base64
