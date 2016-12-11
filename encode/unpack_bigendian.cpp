/*
    A proof of concept: base64 requires to save 4-byte output in big-endian.

    Procedure unpack_with_bswap does two things at once:
    * convert four 6-bit values into 8-bit ones;
    * store them in big-endian order in 32-bit lanes.

    Alfred does it using following sequence (code from AVX2 codec -- https://github.com/aklomp/base64])

            static inline __m256i
            enc_reshuffle (__m256i in)
            {
                // ...
                in = _mm256_shuffle_epi8(in, _mm256_set_epi8(const)
                const __m256i merged = _mm256_blend_epi16(_mm256_slli_epi32(in, 4), in, 0x55);
                const __m256i bd = _mm256_and_si256(merged, _mm256_set1_epi32(0x003F003F));
                const __m256i ac = _mm256_and_si256(_mm256_slli_epi32(merged, 2), _mm256_set1_epi32(0x3F003F00));
                const __m256i indices = _mm256_or_si256(ac, bd);
                return _mm256_bswap_epi32(indices); // bswap pshufb
            }

    Alfred's approach requires:
    * 2 x shuffle
    * 2 x shift
    * 1 x blend
    * 3 x bit ops
    * 4 constants

    This approach:
    * 1 x shuffle
    * 2 x mul
    * 3 x bit ops
    * 5 constants
*/

#include <cstdio>
#include <cstdint>
#include <immintrin.h>


__m128i unpack_with_bswap(const __m128i input) {

    // input = 24 bytes, a single 32-bit lane:
    //       = [????????|ccdddddd|bbbbcccc|aaaaaabb]
    //           byte 3   byte 2   byte 1   byte 0    -- byte 3 comes from the next triplet

    // in    = [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc] - order 1, 0, 2, 1
    //              ^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^
    //                     processed bits
    const __m128i in = _mm_shuffle_epi8(input, _mm_set_epi8(
        10, 11, 9, 10,
         7,  8, 6,  7,
         4,  5, 3,  4,
         1,  2, 0,  1
    ));

    // t0    = [0000cccc|cc000000|aaaaaa00|00000000]
    const __m128i t0 = _mm_and_si128(in, _mm_set1_epi32(0x0fc0fc00));
    // t1    = [00000000|00cccccc|00000000|00aaaaaa]
    //          (c * (1 << 10), a * (1 << 6)) >> 16 (note: an unsigned multiplication)
    const __m128i t1 = _mm_mulhi_epu16(t0, _mm_set1_epi32(0x04000040));

    // t2    = [00000000|00dddddd|000000bb|bbbb0000]
    const __m128i t2 = _mm_and_si128(in, _mm_set1_epi32(0x003f03f0));
    // t3    = [00dddddd|00000000|00bbbbbb|00000000](
    //          (d * (1 << 8), b * (1 << 4))
    const __m128i t3 = _mm_mullo_epi16(t2, _mm_set1_epi32(0x01000010));

    // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
    return _mm_or_si128(t1, t3);
}


uint32_t build_word(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {

    union {
        uint8_t byte[4];
        uint32_t dword;
    };

    byte[0] = (a << 2) | (b >> 4);
    byte[1] = (b << 4) | (c >> 2);
    byte[2] = (d) | (c << 6);
    byte[3] = 0;

    return dword;
}


bool validate() {

    uint8_t tmp[16];

    int a, b, c, d;
    uint32_t in;

    for (a=0; a < 64; a++) {
        for (b=0; b < 64; b++) {
            for (c=0; c < 64; c++) {
                for (d=0; d < 64; d++) {
                    in = build_word(a, b, c, d);
                    const __m128i v = _mm_set1_epi32(in);
                    const __m128i r = unpack_with_bswap(v);

                    _mm_storeu_si128((__m128i*)tmp, r);

                    if (tmp[0] != a) goto error;
                    if (tmp[1] != b) goto error;
                    if (tmp[2] != c) goto error;
                    if (tmp[3] != d) goto error;
                }
            }
        }
    }

    return true;

error:
    printf("failed for %02x %02x %02x %02x (%08x)\n", a, b, c, d, in);
    printf("    result %02x %02x %02x %02x\n", tmp[0], tmp[1], tmp[2], tmp[3]);

    return false;
}


int main() {
    const bool ret = validate();
    if (ret) {
        puts("All OK");
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
