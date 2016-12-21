#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace xop {

    #define packed_byte(x) _mm_set1_epi8(char(x))

        __m128i lookup(const __m128i input) {

            // Even characters from "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/".
            // The difference between codes of adjacent letters is 1, except '+' and '/', for
            // them difference is 4.
            const __m128i base64_lo = _mm_setr_epi8(
                'A', 'C', 'E', 'G', 'I', 'K', 'M', 'O', 'Q', 'S', 'U', 'W', 'Y', 'a', 'c', 'e'
            );

            const __m128i base64_hi = _mm_setr_epi8(
                'g', 'i', 'k', 'm', 'o', 'q', 's', 'u', 'w', 'y', '0', '2', '4', '6', '8', '+'
            );

            // input   = packed_byte(00ab_cdef)
            // bits15  = packed_byte(000a_bcde) -- five highest bits
            const __m128i bits15 = _mm_shl_epi8(input, packed_byte(-1));

            // bit0    = packed_byte(0000_000f) -- the LSB
            const __m128i bit0   = _mm_and_si128(input, packed_byte(0x01));

            // t0      = bits 5:1 translated into ASCII codes
            const __m128i t0     = _mm_perm_epi8(base64_lo, base64_hi, bits15);

            // t1      = the codes adjusted by 0th bit
            const __m128i t1     = _mm_add_epi8(t0, bit0);

            // t3      = special fixup for input == 63
            const __m128i t3     = _mm_and_si128(_mm_cmpeq_epi8(input, packed_byte(63)), packed_byte(3));

            return _mm_add_epi8(t1, t3);
        }

    #undef packed_byte

    } // namespace xop

} // namespace base64


