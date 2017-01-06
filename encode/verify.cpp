#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>

#include "config.h"
#include "encode.scalar.cpp"
#include "lookup.reference.cpp"
#include "lookup.swar.cpp"
#include "encode.swar.cpp"
#if defined(HAVE_SSE_INSTRUCTIONS)
#   include "lookup.sse.cpp"
#   include "encode.sse.cpp"a
#endif
#if defined(HAVE_AVX2_INSTRUCTIONS)
#   include "lookup.avx2.cpp"
#   include "encode.avx2.cpp"
#endif
#if defined(HAVE_AVX512_INSTRUCTIONS)
#   include "../avx512_swar.cpp"
#   include "lookup.avx512.cpp"
#   include "unpack.avx512.cpp"
#   include "encode.avx512.cpp"
#endif
#if defined(HAVE_AVX512BW_INSTRUCTIONS)
#   include "encode.avx512bw.cpp"
#endif
#if defined(HAVE_XOP_INSTRUCTIONS)
#   include "lookup.xop.cpp"
#   include "encode.xop.cpp"
#endif
#if defined(HAVE_NEON_INSTRUCTIONS)
#   include "lookup.neon.cpp"
#   include "encode.neon.cpp"
#endif

const char* lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

template<typename LOOKUP_FN>
void test_scalar(const char* name, LOOKUP_FN fn) {

    printf("%s... ", name);
    fflush(stdout);

    for (unsigned i=0; i < 64; i++) {

        const auto ref = fn(i);
        if (ref != lookup[i]) {

            printf("failed at %d (%d != %d)\n", i, ref, lookup[i]);
            exit(1);
        }
    }

    puts("OK");
}


template<typename LOOKUP_FN>
void test_swar(const char* name, LOOKUP_FN fn) {

    printf("%s... ", name);
    fflush(stdout);

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
                        exit(1);
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        exit(1);
                    }
                }
            }
        }
    }

    puts("OK");
}


#if defined(HAVE_SSE_INSTRUCTIONS)
template<typename LOOKUP_FN>
void test_sse(const char* name, LOOKUP_FN fn) {

    printf("%s... ", name);
    fflush(stdout);

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
                        exit(1);
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        exit(1);
                    }
                }
            }
        }
    }

    puts("OK");
}
#endif


#if defined(HAVE_AVX2_INSTRUCTIONS)
template<typename LOOKUP_FN>
void test_avx2(const char* name, LOOKUP_FN fn) {

    printf("%s... ", name);
    fflush(stdout);

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
                        exit(1);
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        exit(1);
                    }
                }
            }
        }
    }

    puts("OK");
}
#endif


#if defined(HAVE_AVX512_INSTRUCTIONS)
template<typename LOOKUP_FN>
void test_avx512(const char* name, LOOKUP_FN fn) {

    printf("%s... ", name);
    fflush(stdout);

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
                        exit(1);
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        exit(1);
                    }
                }
            }
        }
    }

    puts("OK");
}
#endif


#if defined(HAVE_NEON_INSTRUCTIONS)
template<typename LOOKUP_FN>
void test_neon(const char* name, LOOKUP_FN fn) {

    printf("%s... ", name);
    fflush(stdout);

    const size_t N = 8;

    uint8_t input[N];
    uint8_t output[N];

    for (unsigned byte=0; byte < N; byte++) {

        for (unsigned j=0; j < N; j++) {
            input[j] = 0;
        }

        for (unsigned i=0; i < 64; i++) {

            input[byte] = i;

            uint8x8_t in  = vld1_u8(input);
            uint8x8_t out = fn(in);

            vst1_u8(output, out);

            for (unsigned j=0; j < N; j++) {

                if (j == byte) {
                    if (output[j] != lookup[i]) {
                        printf("failed at %d, byte %d - wrong result\n", i, byte);
                        exit(1);
                    }
                } else {
                    if (output[j] != lookup[0]) {
                        printf("failed at %d, byte %d - spoiled random byte\n", i, byte);
                        exit(1);
                    }
                }
            }
        }
    }

    puts("OK");
}
#endif


int validate_lookup() {

    test_scalar("reference branchless (optimized v2)", reference::lookup_version2);
    test_scalar("reference branchless (naive)", reference::lookup_naive);
    test_scalar("reference branchless (optimized)", reference::lookup_version1);

    test_swar("SWAR (64 bit)", base64::swar::lookup_incremental_logic);

#if defined(HAVE_SSE_INSTRUCTIONS)
    test_sse("SSE implementation of naive algorithm", base64::sse::lookup_naive);
    test_sse("SSE implementation of optimized algorithm (ver 1)", base64::sse::lookup_version1);
    test_sse("SSE implementation of optimized algorithm (ver 2)", base64::sse::lookup_version2);
    test_sse("SSE pshufb-based algorithm", base64::sse::lookup_pshufb);
    test_sse("SSE pshufb improved algorithm", base64::sse::lookup_pshufb_improved);
#endif

#if defined(HAVE_XOP_INSTRUCTIONS)
    test_sse("XOP implementation", base64::xop::lookup);
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
    test_avx2("AVX2 implementation of optimized algorithm", base64::avx2::lookup_version2);
    test_avx2("AVX2 implementation of pshufb-based algorithm", base64::avx2::lookup_pshufb);
    test_avx2("AVX2 implementation of pshufb improved algorithm", base64::avx2::lookup_pshufb_improved);
#endif

#if defined(HAVE_AVX512_INSTRUCTIONS)
    test_avx512("AVX512F (incremental logic)", base64::avx512::lookup_incremental_logic);
    test_avx512("AVX512F (incremental logic improved)", base64::avx512::lookup_incremental_logic_improved);
    test_avx512("AVX512F (binary search)", base64::avx512::lookup_binary_search);
#endif

#if defined(HAVE_NEON_INSTRUCTIONS)
    test_neon("ARM NEON implementation of naive algorithm", base64::neon::lookup_naive);
    test_neon("ARM NEON implementation of optimized algorithm", base64::neon::lookup_version2);
    test_neon("ARM NEON implementation of pshufb improved algorithm", base64::neon::lookup_pshufb_improved);
#endif
    return 0;
}


template <typename ENC>
void validate_encoding(const char* name, ENC encode) {

    const char*  input    = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
                            "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
                            "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
                            "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
                            "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
                            "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f";
    const size_t len      = 96;
    const char*  expected = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v"
                            "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v";
    const size_t outlen   = strlen(expected);
    uint8_t output[512];

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

    validate_encoding("scalar (32-bit)", base64::scalar::encode32);
    validate_encoding("scalar (64-bit)", base64::scalar::encode64);

    validate_encoding("SWAR", base64::swar::encode);

#ifdef HAVE_SSE_INSTRUCTIONS
    auto sse = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        base64::sse::encode(base64::sse::lookup_naive, input, bytes, output);
    };

    auto sse_unrolled = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        base64::sse::encode_unrolled(base64::sse::lookup_naive, input, bytes, output);
    };

    validate_encoding("SSE", sse);
    validate_encoding("SSE (unrolled)", sse_unrolled);
    validate_encoding("SSE (fully unrolled)", base64::sse::encode_full_unrolled);

    #ifdef HAVE_BMI2_INSTRUCTIONS
        auto sse_bmi2 = [](const uint8_t* input, size_t bytes, uint8_t* output) {
            base64::sse::encode_bmi2(base64::sse::lookup_naive, input, bytes, output);
        };

        validate_encoding("SSE + BMI2", sse_bmi2);
    #endif
#endif

#ifdef HAVE_AVX2_INSTRUCTIONS
    auto avx2 = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        base64::avx2::encode(base64::avx2::lookup_pshufb, input, bytes, output);
    };

    auto avx2_unrolled = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        base64::avx2::encode_unrolled(base64::avx2::lookup_pshufb, input, bytes, output);
    };

    validate_encoding("AVX2", avx2);
    validate_encoding("AVX2 (unrolled)", avx2_unrolled);
    #ifdef HAVE_BMI2_INSTRUCTIONS
        auto avx2_bmi2 = [](const uint8_t* input, size_t bytes, uint8_t* output) {
            base64::avx2::encode_bmi2(base64::avx2::lookup_pshufb, input, bytes, output);
        };

        validate_encoding("AVX2 + BMI2", avx2_bmi2);
    #endif
#endif

#ifdef HAVE_AVX512_INSTRUCTIONS
    auto avx512 = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        using namespace base64::avx512;
        encode(lookup_incremental_logic, unpack, input, bytes, output);
    };

    auto avx512_lookup_gather = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        using namespace base64::avx512;
        encode(lookup_gather, unpack_identity, input, bytes, output);
    };

    auto avx512_load_gather = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        using namespace base64::avx512;
        encode_load_gather(lookup_incremental_logic_improved, unpack, input, bytes, output);
    };

    validate_encoding("AVX512", avx512);
    validate_encoding("AVX512 (lookup gather)", avx512_lookup_gather);
    validate_encoding("AVX512 (load gather)", avx512_load_gather);
#endif

#ifdef HAVE_AVX512BW_INSTRUCTIONS
    validate_encoding("AVX512BW", base64::avx512bw::encode);
#endif

#ifdef HAVE_XOP_INSTRUCTIONS
    auto xop = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        base64::xop::encode(base64::xop::lookup, input, bytes, output);
    };

    validate_encoding("XOP", xop);
#endif

#ifdef HAVE_NEON_INSTRUCTIONS
    auto neon = [](const uint8_t* input, size_t bytes, uint8_t* output) {
        base64::neon::encode(base64::neon::lookup_naive, input, bytes, output);
    };

    validate_encoding("ARM NEON", neon);
#endif
}


int main() {

#if defined(HAVE_AVX512_INSTRUCTIONS)
    base64::avx512::initialize();
#endif

    puts("Validate lookup procedures");
    validate_lookup();

    puts("Validate encoding");
    validate_encoding();

    return EXIT_SUCCESS;
}
