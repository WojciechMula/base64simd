#include <cassert>
#if defined(HAVE_BMI_INSTRUCTIONS) || defined(HAVE_BMI2_INSTRUCTIONS)
#   include <x86intrin.h>
#endif

namespace base64 {

    namespace scalar {

        static uint8_t lookup[256];
        static uint32_t lookup32[4][256];

        void prepare_lookup() {

            for (unsigned i=0; i < 256; i++) {
                lookup[i] = 0xff;
            }

            for (unsigned i=0; i < 64; i++) {

                const uint8_t c = static_cast<uint8_t>(base64::lookup[i]);
                lookup[c] = i;
            }
        }

        void decode_lookup1(const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 4 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 4) {

                const uint8_t b0 = base64::scalar::lookup[input[i + 0]];
                if (b0 == 0xff) {
                    throw invalid_input(i + 0, input[i + 0]);
                }

                const uint8_t b1 = base64::scalar::lookup[input[i + 1]];
                if (b1 == 0xff) {
                    throw invalid_input(i + 1, input[i + 1]);
                }

                const uint8_t b2 = base64::scalar::lookup[input[i + 2]];
                if (b2 == 0xff) {
                    throw invalid_input(i + 2, input[i + 2]);
                }

                const uint8_t b3 = base64::scalar::lookup[input[i + 3]];
                if (b3 == 0xff) {
                    throw invalid_input(i + 3, input[i + 3]);
                }

                // Note: character '=' is not handled here.

                // input:  [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
                // output: [00000000|ccdddddd|bbbbcccc|aaaaaabb]

                *out++ = (b1 >> 4) | (b0 << 2);
                *out++ = (b2 >> 2) | (b1 << 4);
                *out++ = b3 | (b2 << 6);
            }
        }

        void prepare_lookup32() {

            for (unsigned j=0; j < 4; j++) {

                uint32_t invalid;

                switch (j) {
                    case 0:
                        invalid = 0x80000000;
                        break;
                    case 1:
                        invalid = 0x81000000;
                        break;
                    case 2:
                        invalid = 0x82000000;
                        break;
                    case 3:
                        invalid = 0x83000000;
                        break;
                }

                for (unsigned i=0; i < 256; i++) {
                    lookup32[j][i] = invalid;
                }

                // output: [00000000|ccdddddd|bbbbcccc|aaaaaabb]
                for (unsigned i=0; i < 64; i++) {

                    const uint8_t c = static_cast<uint8_t>(base64::lookup[i]);
                    uint32_t value;
                    switch (j) {
                        case 0: // a
                            value = i << 2;
                            break;
                        
                        case 1: // b
                            value = (i >> 4) | ((i & 0x0f) << 12);
                            break;

                        case 2: // c
                            value = ((i & 0x3) << 22) | ((i & 0x3c) << 6);
                            break;

                        case 3: // d
                            value = (i << 16);
                            break;
                    }
                    lookup32[j][c] = value;
                }
            }

        }

        void decode_lookup2(const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 4 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 4) {

                const uint32_t dword = lookup32[0][input[i + 0]]
                                     | lookup32[1][input[i + 1]]
                                     | lookup32[2][input[i + 2]]
                                     | lookup32[3][input[i + 3]];

                // Note: character '=' is not handled here.
                if (dword & 0x80000000) {

                    const auto shift = (dword >> 24) & 0x3;
                    throw invalid_input(i + shift, input[i + shift]);
                }

                *reinterpret_cast<uint32_t*>(out) = dword;
                out += 3;

            }
        }

#if defined(HAVE_BMI2_INSTRUCTIONS)
        void decode_lookup1_bmi2(const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % 4 == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 4) {

                const uint8_t b0 = lookup[input[i + 0]];
                if (b0 == 0xff) {
                    throw invalid_input(i + 0, input[i + 0]);
                }

                const uint8_t b1 = lookup[input[i + 1]];
                if (b1 == 0xff) {
                    throw invalid_input(i + 1, input[i + 1]);
                }

                const uint8_t b2 = lookup[input[i + 2]];
                if (b2 == 0xff) {
                    throw invalid_input(i + 2, input[i + 2]);
                }

                const uint8_t b3 = lookup[input[i + 3]];
                if (b3 == 0xff) {
                    throw invalid_input(i + 3, input[i + 3]);
                }

                // Note: character '=' is not handled here.

                // input:  [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
                // output: [00000000|ddddddcc|ccccbbbb|bbaaaaaa]

                const uint32_t combined = (b0 << (3*8)) | (b1 << (2*8)) | (b2 << (1*8)) | (b3);
#ifdef __GNUC__
                const uint32_t dword = __builtin_bswap32(_pext_u32(combined, 0x3f3f3f3f) << 8);
#else

                const uint32_t dword = _bswap(_pext_u32(combined, 0x3f3f3f3f) << 8);
#endif
                *reinterpret_cast<uint32_t*>(out) = dword;
                out += 3;
            }
        }
#endif

        void initialize() {

            prepare_lookup();
            prepare_lookup32();
        }

    } // namespace scalar

} // namespace base64
