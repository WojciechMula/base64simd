// file:///home/wojtek/www/public_html/notesen/2016-04-03-avx512-base64.html#faster-procedure-avx512bw

namespace base64 {

    namespace avx512bw {

        __m512i unpack(const __m512i in) {

            // in    = [bbbbcccc|ccbddddd|aaaaaabb|bbbbcccc]
            //          ^^^^                           ^^^^
            //          ignored                      ignored

            // t0    = [0000cccc|cc000000|aaaaaa00|00000000]
            const __m512i t0 = _mm512_and_si512(in, _mm512_set1_epi32(0x0fc0fc00));

            // t1    = [00000000|00cccccc|00000000|00aaaaaa]
            const __m512i t1 = _mm512_srlv_epi16(t0, _mm512_set1_epi32(0x0006000a));

            // t2    = [ccdddddd|00000000|aabbbbbb|cccc0000]
            const __m512i t2 = _mm512_sllv_epi16(in, _mm512_set1_epi32(0x00080004));

            //         = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
            const int CMOV = 0xca;
            const __m512i indices = _mm512_ternarylogic_epi32(_mm512_set1_epi32(0x3f003f00), t2, t1, CMOV);
        
            return indices;
        }

    } // namespace avx512bw

} // base64

