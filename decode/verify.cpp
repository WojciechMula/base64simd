#include <cstdio>
#include <cstring>

#include "../config.h"

#include "all.cpp"
#include "function_registry.cpp"
#include "initialize.cpp"


class Test {

    uint8_t input[128];
    uint8_t output[128];
    unsigned bytes;
    unsigned out_size;
    unsigned current;

    uint8_t decode_table[256]; // ASCII -> raw value or 0xff if a value is not valid

public:
    Test(unsigned in_size, unsigned out_size)
        : bytes(in_size)
        , out_size(out_size) {

        for (unsigned i=0; i < 256; i++) {
            decode_table[i] = 0xff;
        }

        for (unsigned i=0; i < 64; i++) {
            uint8_t idx = base64::lookup[i];
            decode_table[idx] = i;
        }
    }

public:
    template <typename FN>
    bool run(FN fn) {
        return check(fn, "AAEC", "\x00\x01\x02")
            && check(fn, "PwAA", "\x3f\x00\x00")
            && check(fn, "wA8A", "\xc0\x0f\x00")
            && check(fn, "AA+A", "\x00\x0f\x80")
            && validate(fn);
    }

private:
    template <typename FN>
    bool check(FN fn, const char* in, const char* out) {
        clear_input();

        input[0] = in[0];
        input[1] = in[1];
        input[2] = in[2];
        input[3] = in[3];

        fn(input, bytes, output);

        if (output[0] != uint8_t(out[0]) || output[1] != uint8_t(out[1]) || output[2] != uint8_t(out[2])) {
            printf("\ninvalid output %02x %02x %02x, expected %02x %02x %02x (input: %c%c%c%c)\n",
                        output[0], output[1], output[2],
                        uint8_t(out[0]), uint8_t(out[1]), uint8_t(out[2]),
                        in[0], in[1], in[2], in[3]);
            return false;
        }

        return true;
    }

    template <typename FN>
    bool validate(FN fn) {

        for (current=0; current < bytes; current++) {

            clear_input();

            for (unsigned k=0; k < 256; k++) {
                input[current] = k;

                if (decode_table[k] != 0xff) {

                    try {
                        fn(input, bytes, output);
                    } catch (base64::invalid_input& e) {
                        printf("unexpected error: invalid input byte %c (%02x)\n", e.byte, e.byte);
                        return false;
                    }

                    const bool ok = validate_output(decode_table[k]);
                    if (!ok) {
                        return false;
                    }
                } else {
                    try {
                        fn(input, bytes, output);

                        printf("function should return error for invalid input %02x (at pos %u)\n", k, current);
                        dump_input();
                        return false;
                    } catch (base64::invalid_input& e) {

                        if (e.offset != current) {
                            printf("exception field 'offset' is %lu, should be %u\n", e.offset, current);
                            return false;
                        }

                        if (e.byte != k) {
                            printf("exception field 'byte' is %u, should be %u\n", e.byte, k);
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

private:
    void clear_input() {
        for (unsigned i=0; i < bytes; i++) {
            input[i] = base64::lookup[0];
        }
    }

    bool validate_output(unsigned expected_current) {

        for (unsigned i=0; i < bytes; i++) {

            const uint8_t val = get_6bit_word(i);
            if (i == current) {
                if (val != expected_current) {
                    printf("6-bit field #%d has value %d, expected %d\n", i, val, expected_current);
                    dump_input();
                    dump_output();
                    return false;
                }
            } else {
                if (val != 0) {
                    printf("6-bit field #%d has value %d, expected to be cleared\n", i, val);
                    dump_input();
                    dump_output();
                    return false;
                }
            }
        }

        return true;
    }

    void dump_output() {

        for (int i=out_size - 1; i >= 0; i--) {
            printf("%02x", output[i]);
        }

        putchar('\n');
    }

    void dump_input() {

        for (int i=bytes - 1; i >= 0; i--) {
            printf("%02x", input[i]);
        }

        putchar(' ');

        for (unsigned i=0; i < bytes; i++) {
            printf("%c", input[i]);
        }

        putchar('\n');
    }

    uint8_t get_6bit_word(unsigned i) {

        const unsigned triplet = 3 * (i/4);
        switch (i % 4) {
            case 0: // a
                return output[triplet + 0] >> 2;

            case 1: // b
                return ((output[triplet + 0] & 0x03) << 4)
                     | ((output[triplet + 1] & 0xf0) >> 4);

            case 2: // c
                return ((output[triplet + 1] & 0x0f) << 2)
                     | ((output[triplet + 2] & 0xc0) >> 6);

            case 3: // d
                return output[triplet + 2] & 0x3f;

            default:
                return -1;
        }
    }
};


int test() {

    FunctionRegistry names;
    auto show_name = [names](const char* name) {
        printf("%*s ... ", -names.get_width(), names[name]);
        fflush(stdout);
    };

    const int ANSI_GREEN = 32;

#define RUN_PROCEDURE(input_size, output_size, name, function)  \
    show_name(name);                                            \
    {   Test test(input_size, output_size);                     \
        if (test.run(function)) {                               \
            printf("\033[%dm%s\033[0m\n", ANSI_GREEN, "OK");    \
        } else {                                                \
            return 1;                                           \
        }                                                       \
    }

    RUN_PROCEDURE(4, 3, "scalar",   base64::scalar::decode_lookup1);
    RUN_PROCEDURE(4, 3, "improved", base64::scalar::decode_lookup2);

#if defined(HAVE_BMI2_INSTRUCTIONS)
    RUN_PROCEDURE(4, 3, "scalar_bmi2", base64::scalar::decode_lookup1_bmi2);
#endif // HAVE_BMI2_INSTRUCTIONS

#define RUN_TEMPLATE1(input_size, output_size, name, decode_fn, lookup_fn) { \
        auto function = [](const uint8_t* input, size_t size, uint8_t* output) { \
            return decode_fn(lookup_fn, input, size, output); \
        }; \
        RUN_PROCEDURE(input_size, output_size, name, function) \
    }

#define RUN_TEMPLATE2(input_size, output_size, name, decode_fn, lookup_fn, pack_fn) { \
        auto function = [](const uint8_t* input, size_t size, uint8_t* output) { \
            return decode_fn(lookup_fn, pack_fn, input, size, output); \
        }; \
        RUN_PROCEDURE(input_size, output_size, name, function) \
    }

#define RUN_SSE_TEMPLATE2(name, decode_fn, lookup_fn, pack_fn) \
    RUN_TEMPLATE2(16, 12, name, decode_fn, lookup_fn, pack_fn)

#define RUN_AVX2_TEMPLATE2(name, decode_fn, lookup_fn, pack_fn) \
    RUN_TEMPLATE2(32, 24, name, decode_fn, lookup_fn, pack_fn)

#define RUN_SSE_TEMPLATE1(name, decode_fn, lookup_fn) \
    RUN_TEMPLATE1(16, 12, name, decode_fn, lookup_fn)

#define RUN_AVX2_TEMPLATE1(name, decode_fn, lookup_fn) \
    RUN_TEMPLATE1(32, 24, name, decode_fn, lookup_fn)

#define RUN_NEON_TEMPLATE2(name, decode_fn, lookup_fn) \
    RUN_TEMPLATE1(32, 24, name, decode_fn, lookup_fn)

#define RUN_RVV_VLEN16_LMUL8_TEMPLATE(name, decode_fn, lookup_fn, pack_fn) \
    RUN_TEMPLATE2(16*8, 3*((16*8)/4), name, decode_fn, lookup_fn, pack_fn)

#if defined(HAVE_SSE_INSTRUCTIONS)
    {
    using namespace base64::sse;

    RUN_SSE_TEMPLATE2("sse/1", decode, lookup_base,        pack_naive);
    RUN_SSE_TEMPLATE2("sse/2", decode, lookup_byte_blend,  pack_naive);
    RUN_SSE_TEMPLATE2("sse/3", decode, lookup_incremental, pack_naive);
    RUN_SSE_TEMPLATE2("sse/4", decode, lookup_pshufb,      pack_naive);

    RUN_SSE_TEMPLATE2("sse/5", decode, lookup_base,        pack_madd);
    RUN_SSE_TEMPLATE2("sse/6", decode, lookup_byte_blend,  pack_madd);
    RUN_SSE_TEMPLATE2("sse/7", decode, lookup_incremental, pack_madd);
    RUN_SSE_TEMPLATE2("sse/8", decode, lookup_pshufb,      pack_madd);
    RUN_SSE_TEMPLATE2("sse/9", decode, lookup_pshufb_bitmask, pack_madd);

    RUN_PROCEDURE(16, 12, "sse/10", decode_aqrit);
    }
#endif

#if defined(HAVE_XOP_INSTRUCTIONS)
    {
    RUN_SSE_TEMPLATE2("xop", base64::sse::decode, base64::xop::lookup_pshufb_bitmask, base64::sse::pack_madd);
    }
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
    {
    using namespace base64::sse;

    RUN_SSE_TEMPLATE1("sse_bmi2/1", decode_bmi2, lookup_base);
    RUN_SSE_TEMPLATE1("sse_bmi2/2", decode_bmi2, lookup_byte_blend);
    RUN_SSE_TEMPLATE1("sse_bmi2/3", decode_bmi2, lookup_incremental);
    }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
    {
    using namespace base64::avx2;

    RUN_AVX2_TEMPLATE2("avx2/1", decode, lookup_base,        pack_naive);
    RUN_AVX2_TEMPLATE2("avx2/2", decode, lookup_byte_blend,  pack_naive);
    RUN_AVX2_TEMPLATE2("avx2/3", decode, lookup_pshufb,      pack_naive);

    RUN_AVX2_TEMPLATE2("avx2/4", decode, lookup_base,        pack_madd);
    RUN_AVX2_TEMPLATE2("avx2/5", decode, lookup_byte_blend,  pack_madd);
    RUN_AVX2_TEMPLATE2("avx2/6", decode, lookup_pshufb,      pack_madd);
    RUN_AVX2_TEMPLATE2("avx2/7", decode, lookup_pshufb_bitmask, pack_madd);
    }

    #if defined(HAVE_BMI2_INSTRUCTIONS)
    {
    using namespace base64::avx2;

    RUN_AVX2_TEMPLATE1("avx2_bmi2/1", decode_bmi2, lookup_base);
    RUN_AVX2_TEMPLATE1("avx2_bmi2/2", decode_bmi2, lookup_byte_blend);
    }
    #endif // HAVE_BMI2_INSTRUCTIONS

#endif // HAVE_AVX2_INSTRUCTIONS

#if defined(HAVE_AVX512_INSTRUCTIONS)
    {
    using namespace base64::avx512;
    RUN_TEMPLATE2(64, 48, "avx512/1", decode, lookup_gather,      pack_identity);
    RUN_TEMPLATE2(64, 48, "avx512/2", decode, lookup_comparisons, pack_improved);
    }
#endif // HAVE_AVX512_INSTRUCTIONS

#if defined(HAVE_AVX512BW_INSTRUCTIONS)
    {
    using namespace base64::avx512bw;
    RUN_TEMPLATE2(64, 48, "avx512bw/1", decode, lookup_pshufb_bitmask, pack_madd)
    RUN_TEMPLATE2(64, 48, "avx512bw/2", decode, lookup_aqrit, pack_madd)
    }
#endif // HAVE_AVX512VBMI_INSTRUCTIONS

#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
    {
    using namespace base64::avx512vbmi;
    RUN_TEMPLATE2(64, 48, "avx512vbmi", decode, lookup, base64::avx512bw::pack_madd)
    }
#endif // HAVE_AVX512VBMI_INSTRUCTIONS

#if defined(HAVE_NEON_INSTRUCTIONS)
    {
    using namespace base64::neon;
    RUN_NEON_TEMPLATE2("neon/1", decode, lookup_byte_blend);
    RUN_NEON_TEMPLATE2("neon/2", decode, lookup_pshufb_bitmask);
    }
#endif // HAVE_NEON_INSTRUCTIONS

#if defined(HAVE_RVV_INSTRUCTIONS)
    {
    using namespace base64::rvv;
    RUN_RVV_VLEN16_LMUL8_TEMPLATE("rvv/1", decode_vlen16_m8, lookup_vlen16_m8, pack_vlen16_m8);
    }
#endif // HAVE_RVV_INSTRUCTIONS

    return 0;
}


int main() {
    initialize();

    try {
        return test();
    } catch (base64::invalid_input& e) {

        printf("invalid input byte %c (%02x)\n", e.byte, e.byte);
        return 2;
    }
}
