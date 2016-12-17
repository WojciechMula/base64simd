#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cassert>

#include "config.h"
#include "encode.scalar.cpp"
#include "lookup.reference.cpp"
#include "lookup.sse.cpp"
#include "lookup.swar.cpp"
#if defined(HAVE_AVX2_INSTRUCTIONS)
#   include "lookup.avx2.cpp"
#endif
#if defined(HAVE_AVX512_INSTRUCTIONS)
#   include "../avx512_swar.cpp"
#   include "lookup.avx512.cpp"
#   include "unpack.avx512.cpp"
#endif
#if defined(HAVE_XOP_INSTRUCTIONS)
#   include "lookup.xop.cpp"
#endif

const char* lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

template<typename LOOKUP_FN>
bool test_scalar(LOOKUP_FN fn) {

    for (unsigned i=0; i < 64; i++) {

        const auto ref = fn(i);
        if (ref != lookup[i]) {

            printf("failed at %d (%d != %d)\n", i, ref, lookup[i]);
            return false;
        }
    }

    return true;
}


template<typename LOOKUP_FN>
bool test_swar(LOOKUP_FN fn) {

    uint8_t input[8];
    uint8_t output[8];

    for (unsigned byte=0; byte < 8; byte++) {

        for (unsigned j=0; j < 8; j++) {
            input[j] = 0;
        }

        for (unsigned i=0; i < 64; i++) {

            input[byte] = i;

            uint64_t in;
            uint64_t out;

            memcpy(&in, input, 8);

            out = fn(in);

            memcpy(output, &out, 8);

            for (unsigned j=0; j < 8; j++) {

                if (j == byte) {
                    if (output[j] != lookup[i]) {
                        printf("failed at %d, byte %d - wrong result\n", i, byte);
                        return false;
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        return false;
                    }
                }
            }
        }
    }

    return true;
}


template<typename LOOKUP_FN>
bool test_sse(LOOKUP_FN fn) {

    uint8_t input[16];
    uint8_t output[16];

    for (unsigned byte=0; byte < 16; byte++) {

        for (unsigned j=0; j < 16; j++) {
            input[j] = 0;
        }

        for (unsigned i=0; i < 64; i++) {

            input[byte] = i;

            __m128i in  = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input));
            __m128i out = fn(in);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(output), out);

            for (unsigned j=0; j < 16; j++) {

                if (j == byte) {
                    if (output[j] != lookup[i]) {
                        printf("failed at %d, byte %d - wrong result\n", i, byte);
                        return false;
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        return false;
                    }
                }
            }
        }
    }

    return true;
}


#if defined(HAVE_AVX2_INSTRUCTIONS)
template<typename LOOKUP_FN>
bool test_avx2(LOOKUP_FN fn) {

    uint8_t input[32];
    uint8_t output[32];

    for (unsigned byte=0; byte < 32; byte++) {

        for (unsigned j=0; j < 32; j++) {
            input[j] = 0;
        }

        for (unsigned i=0; i < 64; i++) {

            input[byte] = i;

            __m256i in  = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input));
            __m256i out = fn(in);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(output), out);

            for (unsigned j=0; j < 32; j++) {

                if (j == byte) {
                    if (output[j] != lookup[i]) {
                        printf("failed at %d, byte %d - wrong result\n", i, byte);
                        printf("%02x != %02x (%c != %c)\n", output[j], lookup[i], output[j], lookup[i]);
                        return false;
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        return false;
                    }
                }
            }
        }
    }

    return true;
}
#endif


#if defined(HAVE_AVX512_INSTRUCTIONS)
template<typename LOOKUP_FN>
bool test_avx512(LOOKUP_FN fn) {

    base64::avx512::initialize();

    uint8_t input[64];
    uint8_t output[64];

    for (unsigned byte=0; byte < 64; byte++) {

        for (unsigned j=0; j < 64; j++) {
            input[j] = 0;
        }

        for (unsigned i=0; i < 64; i++) {

            input[byte] = i;

            __m512i in  = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input));
            __m512i out = fn(in);

            _mm512_storeu_si512(reinterpret_cast<__m512i*>(output), out);

            for (unsigned j=0; j < 64; j++) {

                if (j == byte) {
                    if (output[j] != lookup[i]) {
                        printf("failed at %d, byte %d - wrong result\n", i, byte);
                        printf("%02x != %02x (%c != %c)\n", output[j], lookup[i], output[j], lookup[i]);
                        return false;
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        return false;
                    }
                }
            }
        }
    }

    return true;
}


template<typename UNPACK_FN>
bool test_avx512_unpack(UNPACK_FN unpack) {

    base64::avx512::initialize();

    uint8_t input[64];
    uint8_t output[64];
    uint64_t* first_qword = reinterpret_cast<uint64_t*>(input);

    for (unsigned byte=0; byte < 4; byte++) {

        for (unsigned j=0; j < 64; j++) {
            input[j] = 0;
        }

        for (int i=0; i < 64; i++) {

            *first_qword = uint64_t(i) << (byte * 6);

            __m512i in  = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input));
            __m512i out = unpack(in);

            _mm512_storeu_si512(reinterpret_cast<__m512i*>(output), out);

            for (unsigned j=0; j < 4; j++) {

                if (j == byte) {
                    if (output[j] != i) {
                        printf("failed at %d, byte %d - wrong result\n", i, byte);
                        return false;
                    }
                } else {
                    if (output[j] != 0) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        return false;
                    }
                }
            }
        }
    }

    return true;
}
#endif


int test() {

    printf("reference branchless (optimized v2)... ");
    fflush(stdout);
    if (test_scalar(reference::lookup_version2)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("reference branchless (naive)... ");
    fflush(stdout);
    if (test_scalar(reference::lookup_naive)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("reference branchless (optimized)... ");
    fflush(stdout);
    if (test_scalar(reference::lookup_version1)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("SWAR (64 bit)... ");
    fflush(stdout);
    if (test_swar(base64::swar::lookup_incremental_logic)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("SSE implementation of naive algorithm... ");
    fflush(stdout);
    if (test_sse(base64::sse::lookup_naive)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("SSE implementation of optimized algorithm (ver 1)... ");
    fflush(stdout);
    if (test_sse(base64::sse::lookup_version1)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("SSE implementation of optimized algorithm (ver 2)... ");
    fflush(stdout);
    if (test_sse(base64::sse::lookup_version2)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("SSE pshufb-based algorithm... ");
    fflush(stdout);
    if (test_sse(base64::sse::lookup_pshufb)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("SSE pshufb improved algorithm... ");
    fflush(stdout);
    if (test_sse(base64::sse::lookup_pshufb_improved)) {
        puts("OK");
    } else {
        return 1;
    }

#if defined(HAVE_XOP_INSTRUCTIONS)
    printf("XOP implementation... ");
    fflush(stdout);
    if (test_sse(base64::xop::lookup)) {
        puts("OK");
    } else {
        return 1;
    }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
    printf("AVX2 implementation of optimized algorithm... ");
    fflush(stdout);
    if (test_avx2(base64::avx2::lookup_version2)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("AVX2 implementation of pshufb-based algorithm... ");
    fflush(stdout);
    if (test_avx2(base64::avx2::lookup_pshufb)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("AVX2 implementation of pshufb improved algorithm... ");
    fflush(stdout);
    if (test_avx2(base64::avx2::lookup_pshufb_improved)) {
        puts("OK");
    } else {
        return 1;
    }
#endif

#if defined(HAVE_AVX512_INSTRUCTIONS)
    printf("AVX512F lookup:\n");
    printf("AVX512F (incremental logic)... ");
    fflush(stdout);

    if (test_avx512(base64::avx512::lookup_incremental_logic)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("AVX512F (incremental logic improved)... ");
    fflush(stdout);

    if (test_avx512(base64::avx512::lookup_incremental_logic_improved)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("AVX512F (binary search)... ");
    fflush(stdout);

    if (test_avx512(base64::avx512::lookup_binary_search)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("AVX512F unpack:\n");
    printf("AVX512F (default)... ");
    fflush(stdout);

    if (test_avx512_unpack(base64::avx512::unpack_default)) {
        puts("OK");
    } else {
        return 1;
    }

    printf("AVX512F (improved)... ");
    fflush(stdout);

    if (test_avx512_unpack(base64::avx512::unpack_improved)) {
        puts("OK");
    } else {
        return 1;
    }
#endif
    return 0;
}


template <typename ENC>
void validate_encoding(const char* name, ENC encode) {
    
    const char*  input    = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
                            "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
                            "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f";
    const size_t len      = 48;
    const char*  expected = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v";
    const size_t outlen   = strlen(expected);
    uint8_t output[256];

    printf("%s... ", name);
    fflush(stdout);

    encode(reinterpret_cast<const uint8_t*>(input), len, output);

    if (memcmp(expected, output, outlen) != 0) {
        puts("FAILED");
        printf("expected: '%s'\n", expected);
        printf("result:   '%s'\n", output);
        exit(1);
    }

    puts("OK");
}


void validate_encoding() {
 
    puts("Validate encoding");
    validate_encoding("scalar (32-bit)", base64::scalar::encode32);
    validate_encoding("scalar (64-bit)", base64::scalar::encode64);
}


int main() {

    /*if (!test()) {
        return EXIT_FAILURE;
    }*/

    validate_encoding();

    return EXIT_SUCCESS;
}
