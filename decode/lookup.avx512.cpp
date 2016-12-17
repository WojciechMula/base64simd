#include <cstdint>
#include <cassert>
#include <stdexcept>

#include <immintrin.h>
#include <x86intrin.h>

#define packed_dword(x) _mm512_set1_epi32(x)
#define packed_byte(x) packed_dword((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))

namespace base64 {

    namespace avx512 {

        namespace precalc {

            static uint8_t valid[256];
            static uint32_t lookup_0[256];
            static uint32_t lookup_1[256];
            static uint32_t lookup_2[256];
            static uint32_t lookup_3[256];
        }

        void initalize_lookup() {

            using namespace precalc;

            for (int i=0; i < 256; i++) {
                lookup_0[i] = 0x80000000;
                lookup_1[i] = 0x80000000;
                lookup_2[i] = 0x80000000;
                lookup_3[i] = 0x80000000;
                valid[i] = 0;
            }

            for (int i=0; i < 64; i++) {
                const uint32_t val = static_cast<uint8_t>(base64::lookup[i]);

                valid[val] = 1;
                lookup_0[val] = i << 2;
                lookup_1[val] = (i >> 4) | ((i & 0x0f) << 12);
                lookup_2[val] = ((i & 0x3) << 22) | ((i & 0x3c) << 6);
                lookup_3[val] = (i << 16);
            }
        }


        void report_exception(const __m512i erroneous_input) {

            // an error occurs just once, peformance is not cruical

            uint8_t tmp[64];
            _mm512_storeu_si512(reinterpret_cast<__m512*>(tmp), erroneous_input);

            for (unsigned i=0; i < 64; i++) {
                if (!precalc::valid[tmp[i]]) {
                    throw invalid_input(i, tmp[i]);
                }
            }

            putchar('\n');
            printf("input: ");
            for (unsigned i=0; i < 64; i++) {
                printf("%02x", tmp[i]);
            }
            putchar('\n');

            printf("ASCII: ");
            for (unsigned i=0; i < 64; i++) {
                printf("%c", tmp[i]);
            }
            putchar('\n');

            throw std::logic_error("the lookup procedure claims there's an error, but the input looks OK");
        }


        const uint8_t OR_ALL = 0xfe;


        __m512i inline __attribute__((always_inline)) lookup_gather(const __m512i input) {

            const __m512i b0 = _mm512_and_si512(input, packed_dword(0x000000ff));
            const __m512i b1 = _mm512_and_si512(_mm512_srli_epi32(input, 1*8), packed_dword(0x000000ff));
            const __m512i b2 = _mm512_and_si512(_mm512_srli_epi32(input, 2*8), packed_dword(0x000000ff));
            const __m512i b3 = _mm512_srli_epi32(input, 3*8);

            // do lookup
            const __m512i r0 = _mm512_i32gather_epi32(b0, (const int*)precalc::lookup_0, 4);
            const __m512i r1 = _mm512_i32gather_epi32(b1, (const int*)precalc::lookup_1, 4);
            const __m512i r2 = _mm512_i32gather_epi32(b2, (const int*)precalc::lookup_2, 4);
            const __m512i r3 = _mm512_i32gather_epi32(b3, (const int*)precalc::lookup_3, 4);

            // r0 | r1 | r2 | r3
            const __m512i translated = _mm512_or_si512(r0, _mm512_ternarylogic_epi32(r1, r2, r3, OR_ALL));

            const uint16_t mask = _mm512_cmplt_epi32_mask(translated, _mm512_set1_epi32(0));
            if (mask) {
                report_exception(input);
            }

            return translated;
        }


        __m512i inline __attribute__((always_inline)) lookup_comparisons(const __m512i input) {

            using namespace avx512f_swar;

            // we operate on lower 7 bits, as all values with 8th bit set are invalid
            const __m512i in = _mm512_and_si512(input, packed_byte(0x7f));

#define GET_RANGE_SHIFT_7BIT(shift, lo, hi) \
            _mm512_and_si512(packed_byte(uint8_t(shift)), \
                             _mm512_range_mask_7bit(in, packed_byte(0x80 - (lo)), packed_byte(0x80 - (hi))))

#define GET_RANGE_SHIFT_8BIT(shift, lo, hi) \
            _mm512_and_si512(packed_byte(uint8_t(shift)), \
                             _mm512_range_mask_8bit(in, packed_byte(0x80 - (lo)), packed_byte(0x80 - (hi))))

#define GET_RANGE_SHIFT_3RD_BIT(lo, hi) \
            _mm512_range_3rd_bit(in, packed_byte(0x80 - (lo)), packed_byte(0x80 - (hi)))


            // shift for range 'A' - 'Z'
            const __m512i range_AZ = GET_RANGE_SHIFT_8BIT(-65, 'A', 'Z' + 1);

            // shift for range 'a' - 'z'
            const __m512i range_az = GET_RANGE_SHIFT_8BIT(-71, 'a', 'z' + 1);

            // shift for range '0' - '9'
            const __m512i range_09 = GET_RANGE_SHIFT_3RD_BIT('0', '9' + 1); // shift = 4

            // shift for character '+'
            const __m512i char_plus = GET_RANGE_SHIFT_7BIT(19, '+', '+' + 1);

            // shift for character '/'
            const __m512i char_slash = GET_RANGE_SHIFT_7BIT(16, '/', '/' + 1);

            // shift = range_AZ | range_az | range_09 | char_plus | char_slash
            const __m512i tmp   = _mm512_ternarylogic_epi32(range_AZ, range_az, range_09, OR_ALL);
            const __m512i shift = _mm512_ternarylogic_epi32(char_plus, char_slash, tmp,   OR_ALL);

            // a paddb equivalent: see ../avx512_swar.cpp:_mm512_add_epu8
            const __m512i MSB = packed_byte(0x80);
            const __m512i shift06 = _mm512_and_si512(shift, packed_byte(0x7f));
            const __m512i result  = _mm512_ternarylogic_epi32(MSB, shift, _mm512_add_epi32(in, shift06), 0x6a);

            // validation
            const __m512i non_zero_7lower = _mm512_add_epi32(shift06, packed_byte(0x7f));

            /*  7 lower bits of shift are non-zero
                | 7th bit of shift is non-zero
                | | 7th bit of input is non-zero (extended ASCII)
                | | |
                a b c | expression = (a | b) & ~c
                ------+---------------------------
                0 0 0 | 0
                0 0 1 | 0
                0 1 0 | 1
                0 1 1 | 0
                1 0 0 | 1
                1 0 1 | 0
                1 1 0 | 1
                1 1 1 | 0
            */
            // we're using 7th bit of each byte
            const __m512i valid = _mm512_ternarylogic_epi32(non_zero_7lower, shift, input, 0x54);

            const auto mask = _mm512_cmpneq_epi32_mask(MSB, _mm512_and_si512(valid, MSB));
            if (mask) {
                report_exception(input);
            }

            return result;
        }

    } // namespace avx512

} // namespace base64


#undef packed_dword
