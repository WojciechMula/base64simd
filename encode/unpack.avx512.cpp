namespace base64 {

    namespace avx512 {

#define packed_dword(x) _mm512_set1_epi32(x)

        __m512i unpack_identity(const __m512i in) {
            return in;
        }


        template <int shift, uint32_t mask>
        __m512i merge(__m512i target, __m512i src) {
            __m512i shifted;
            if (shift > 0) {
                shifted = _mm512_srli_epi32(src, shift);
            } else {
                shifted = _mm512_slli_epi32(src, -shift);
            }

            /*
                mask  shifted  target    res
                 0       0       0        0
                 0       0       1        1
                 0       1       0        0
                 0       1       1        1
                 1       0       0        0
                 1       0       1        0
                 1       1       0        1
                 1       1       1        1

                 mask ? shifted : target
            */

            return _mm512_ternarylogic_epi32(_mm512_set1_epi32(mask), shifted, target, 0xca);
        }

        __m512i unpack(const __m512i in) {
            // [????????|ccdddddd|bbbbCCCC|aaaaaaBB]
            //           ^^       ^^^^^^^^       ^^
            //           lo        lo  hi        hi

            // [00000000|00000000|00000000|00aaaaaa]
            __m512i indices = _mm512_and_si512(_mm512_srli_epi32(in, 2), packed_dword(0x0000003f));

            // [00000000|00000000|00BB0000|00aaaaaa]
            indices = merge<-12, 0x00003000>(indices, in);

            // [00000000|00000000|00BBbbbb|00aaaaaa]
            indices = merge<  4, 0x00000f00>(indices, in);

            // [00000000|00CCCC00|00BBbbbb|00aaaaaa]
            indices = merge<-10, 0x003c0000>(indices, in);

            // [00000000|00CCCCcc|00BBbbbb|00aaaaaa]
            indices = merge<  6, 0x00030000>(indices, in);

            // [00dddddd|00CCCCcc|00BBbbbb|00aaaaaa]
            indices = merge< -8, 0x3f000000>(indices, in);

            return indices;
        }

#undef packed_dword

    } // namespace avx512

} // namespace base64
