/*
    Pack algorithm is responsible for converting from
    four 6-bit indices saved in separate bytes of 32-bit word
    into 24-bit word
*/

namespace base64 {

    namespace avx2 {

#define packed_dword(x) _mm256_set1_epi32(x)
#define masked(x, mask) _mm256_and_si256(x, packed_dword(mask))

        __m256i pack_naive(const __m256i values) {

            const __m256i ca = masked(values, 0x003f003f);
            const __m256i db = masked(values, 0x3f003f00);

            const __m256i t0 = _mm256_or_si256(
                                _mm256_srli_epi32(db, 8),
                                _mm256_slli_epi32(ca, 6)
                               );

            const __m256i t1 = _mm256_or_si256(
                                _mm256_srli_epi32(t0, 16),
                                _mm256_slli_epi32(t0, 12)
                               );

            return masked(t1, 0x00ffffff);
        }


        __m256i pack_madd(const __m256i values) {

            const __m256i merge_ab_and_bc = _mm256_maddubs_epi16(values, packed_dword(0x01400140));

            return _mm256_madd_epi16(merge_ab_and_bc, packed_dword(0x00011000));
         }

#undef packed_dword
#undef masked

    } // namespace sse

} // namespace base64
