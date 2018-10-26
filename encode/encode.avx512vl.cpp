// This file is a copy of encode.avx512bw.cpp with necessary changes to
// use VPMULTISHIFTQB instruction

namespace base64 {

    namespace avx512vl {

        void encode(const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            static const char* lookup_tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            static uint8_t shuffle_input_tbl[64];
            static bool initialized = false;
            if (!initialized) {
                // 32-bit input: [00000000|ccdddddd|bbbbcccc|aaaaaabb]
                //                            2         1        0
                // output order  [1, 2, 0, 1], i.e.:
                //               [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
                unsigned src_index = 0;
                for (int i=0; i < 64; i += 4) {
                    shuffle_input_tbl[i + 0] = src_index + 1;
                    shuffle_input_tbl[i + 1] = src_index + 0;
                    shuffle_input_tbl[i + 2] = src_index + 2;
                    shuffle_input_tbl[i + 3] = src_index + 1;
                    src_index += 3;
                }

                initialized = true;
            }

            const __m512i shuffle_input = _mm512_loadu_si512(reinterpret_cast<__m512i*>(shuffle_input_tbl));
            const __m512i lookup = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(lookup_tbl));

#define packed_qword(x) _mm512_set1_epi64(x)

            for (size_t i = 0; i < bytes; i += 4 * 12) {
                const __m512i v = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));

                // reorder bytes
                // [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
                const __m512i in = _mm512_permutexvar_epi8(shuffle_input, v);

                // after multishift a single 32-bit lane has following layout:
                // [bbbbcccc|bbcccccc|aabbbbbb|ddaaaaaa],
                // i.e.: (a = [10:17], b = [4:11], c = [22:27], d = [16:21])

                const __m512i shifts  = packed_qword(0x3036242a1016040alu); // 48, 54, 36, 42, 16, 22, 4, 10
                const __m512i indices = _mm512_multishift_epi64_epi8(shifts, in);

                // Note: the two higher bits of each indices' byte have garbage,
                //       but the following permutexvar instruction masks them out.

                // translation 6-bit values -> ASCII
                const __m512i result = _mm512_permutexvar_epi8(indices, lookup);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), result);

#undef packed_qdword

                out += 64;
            }
        }

    } // namespace avx512

} // namespace base64
