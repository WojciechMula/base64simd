#include <cstdint>
#include <cassert>

#include <immintrin.h>
#include <x86intrin.h>

namespace base64 {

    namespace avx512 {

        namespace precalc {

            static uint8_t valid[256];
            static __m512i scatter_offsets;
        }

        void initalize_decode() {

            // initalize scatter_offsets
            uint32_t lookup[16];

            for (int i=0; i < 16; i++) {
                lookup[i] = i * 3;
            }

            precalc::scatter_offsets = _mm512_loadu_si512(reinterpret_cast<__m512i*>(lookup));

            // initialize valid
            for (int i=0; i < 256; i++) {
                precalc::valid[i] = 0;
            }

            for (int i=0; i < 64; i++) {
                const uint32_t val = static_cast<uint8_t>(base64::lookup[i]);
                precalc::valid[val] = 1;
            }
        }


        void report_exception(size_t offset, const __m512i erroneous_input) {

            // an error occurs just once, peformance is not cruical

            uint8_t tmp[64];
            _mm512_storeu_si512(reinterpret_cast<__m512*>(tmp), erroneous_input);

            for (unsigned i=0; i < 64; i++) {
                if (!precalc::valid[tmp[i]]) {
                    throw invalid_input(offset + i, tmp[i]);
                }
            }
        }


        template <typename FN_LOOKUP, typename FN_PACK>
        void decode(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 64 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 64) {

                __m512i in = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));
                __m512i values;
                __mmask16 errors;

                values = lookup(in, errors);
                if (errors) {
                    report_exception(i, in);
                }

                const __m512i packed = pack(values);

                _mm512_i32scatter_epi32(reinterpret_cast<int*>(out), precalc::scatter_offsets, packed, 1);

                out += 48;
            }
        }

    } // namespace avx512bw

} // namespace base64
