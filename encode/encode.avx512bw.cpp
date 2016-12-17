#include <immintrin.h>
#include <x86intrin.h>

#include <cstdint>

namespace base64 {

    namespace avx512bw {

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

#define packed_dword(x) _mm512_set1_epi32(x)

            for (size_t i = 0; i < bytes; i += 4 * 12) {
                const __m512i v = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));
                
                // reorder bytes
                const __m512i in = _mm512_permutexvar_epi8(shuffle_input, v);

#if 0
                // unpack procedure from encode.sse.cpp
                const __m512i t0 = _mm512_and_si512(in, _mm512_set1_epi32(0x0fc0fc00));
                const __m512i t1 = _mm512_mulhi_epu16(t0, _mm512_set1_epi32(0x04000040));
                const __m512i t2 = _mm512_and_si512(in, _mm512_set1_epi32(0x003f03f0));
                const __m512i t3 = _mm512_mullo_epi16(t2, _mm512_set1_epi32(0x01000010));
                const __m512i indices = _mm512_or_si512(t1, t3);
#else
                // similar to the above proc, but use variable shifts rather multiplication
                // in    = [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]

                // t0    = [0000cccc|cc000000|aaaaaa00|00000000]
                const __m512i t0 = _mm512_and_si512(in, _mm512_set1_epi32(0x0fc0fc00));
                // t1    = [00000000|00cccccc|00000000|00aaaaaa] (c >> 6, a >> 10)
                const __m512i t1 = _mm512_srlv_epi16(t0, _mm512_set1_epi32(0x0006000a));

                // t2    = [ccdddddd|00000000|aabbbbbb|cccc0000]
                const __m512i t2 = _mm512_sllv_epi16(in, _mm512_set1_epi32(0x00080004));

                // indices = 0x3f003f00 ? t2 : t1
                //         = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]   
                const __m512i indices = _mm512_ternarylogic_epi32(_mm512_set1_epi32(0x3f003f00), t2, t1, 0xca);
#endif
                // translation
                const __m512i result = _mm512_permutexvar_epi8(indices, lookup);

                _mm512_storeu_si512(reinterpret_cast<__m512i*>(out), result);

#undef packed_dword
                out += 64;
            }
        }

    #undef packed_dword

    } // namespace avx512

} // namespace base64
