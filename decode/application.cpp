#include "../cmdline.cpp"
#include "../benchmark.h"
#include "function_registry.cpp"
#include "all.cpp"
#include "initialize.cpp"
#include <cstring>

#ifndef BUFFER_SIZE
#   define BUFFER_SIZE (64*1024*1024)
#endif

#ifndef ITERATIONS
#   define ITERATIONS (10)
#endif

template <typename Derived>
class ApplicationBase {

protected:
    const CommandLine& cmd;
    const FunctionRegistry& names;
    unsigned count;
    unsigned iterations;
    bool initialized;
    bool quiet;

    std::unique_ptr<uint8_t> input;
    std::unique_ptr<uint8_t> output;
public:
    ApplicationBase(const CommandLine& c, const FunctionRegistry& names)
        : cmd(c)
        , names(names)
        , count(BUFFER_SIZE)
        , iterations(ITERATIONS)
        , initialized(false)
        , quiet(false) {}

protected:
    void initialize() {

        if (initialized) {
            return;
        }

        ::initialize();

        input.reset (new uint8_t[get_input_size()]);
        output.reset(new uint8_t[get_output_size() + 128]);

        fill_input();
        custom_initialize();

        initialized = true;
    }

    virtual void custom_initialize() {
        // nop
    }

    size_t get_input_size() const {
        return count;
    }

    size_t get_output_size() const {
        return (3*count)/4;
    }

    void fill_input() {
        for (unsigned i=0; i < get_input_size(); i++) {
            const uint8_t idx = i * 71;
            input.get()[i] = base64::lookup[idx % 64];
        }
    }

    void clear_output() {
        memset(output.get(), 0, get_output_size());
    }

    void dump_output() {
        putchar('\n');
        const size_t N = get_output_size();
        for (size_t i=0; i < N; i++) {
            printf("%02x", output.get()[i]);
        }
        putchar('\n');
    }

protected:
    void run_all() {

        run_function("scalar", base64::scalar::decode_lookup2);
        run_function("improved", base64::scalar::decode_lookup2);

#if defined(HAVE_BMI2_INSTRUCTIONS)
        run_function("scalar_bmi2", base64::scalar::decode_lookup1_bmi2);
#endif

#define RUN_TEMPLATE1(name, decode_fn, lookup_fn) { \
        auto function = [](const uint8_t* input, size_t size, uint8_t* output) { \
            return decode_fn(lookup_fn, input, size, output); \
        }; \
        run_function(name, function); \
    }

#define RUN_TEMPLATE2(name, decode_fn, lookup_fn, pack_fn) \
        if (cmd.empty() || cmd.has(name)) { \
            auto function = [](const uint8_t* input, size_t size, uint8_t* output) { \
                return decode_fn(lookup_fn, pack_fn, input, size, output); \
            }; \
            run_function(name, function); \
        }


#if defined(HAVE_SSE_INSTRUCTIONS)
        {
        using namespace base64::sse;

        RUN_TEMPLATE2("sse/1", decode, lookup_base,        pack_naive);
        RUN_TEMPLATE2("sse/2", decode, lookup_byte_blend,  pack_naive);
        RUN_TEMPLATE2("sse/3", decode, lookup_incremental, pack_naive);
        RUN_TEMPLATE2("sse/4", decode, lookup_pshufb,      pack_naive);

        RUN_TEMPLATE2("sse/5", decode, lookup_base,           pack_madd);
        RUN_TEMPLATE2("sse/6", decode, lookup_byte_blend,     pack_madd);
        RUN_TEMPLATE2("sse/7", decode, lookup_incremental,    pack_madd);
        RUN_TEMPLATE2("sse/8", decode, lookup_pshufb,         pack_madd);
        RUN_TEMPLATE2("sse/9", decode, lookup_pshufb_bitmask, pack_madd);

        run_function("sse/10", decode_aqrit);
        }
#endif // defined(HAVE_SSE_INSTRUCTIONS)

#if defined(HAVE_XOP_INSTRUCTIONS)
        RUN_TEMPLATE2("xop", base64::sse::decode, base64::xop::lookup_pshufb_bitmask, base64::sse::pack_madd);
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
        {
        using namespace base64::sse;

        RUN_TEMPLATE1("sse_bmi2/1", decode_bmi2, lookup_base);
        RUN_TEMPLATE1("sse_bmi2/2", decode_bmi2, lookup_byte_blend);
        RUN_TEMPLATE1("sse_bmi2/3", decode_bmi2, lookup_incremental);
        }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
        {
        using namespace base64::avx2;

        RUN_TEMPLATE2("avx2/1",    decode, lookup_base,        pack_naive);
        RUN_TEMPLATE2("avx2/2",    decode, lookup_byte_blend,  pack_naive);
        RUN_TEMPLATE2("avx2/3",    decode, lookup_pshufb,      pack_naive);

        RUN_TEMPLATE2("avx2/4",    decode, lookup_base,        pack_madd);
        RUN_TEMPLATE2("avx2/5",    decode, lookup_byte_blend,  pack_madd);
        RUN_TEMPLATE2("avx2/6",    decode, lookup_pshufb,      pack_madd);
        RUN_TEMPLATE2("avx2/7",    decode, lookup_pshufb_bitmask, pack_madd);
        }

#   if defined(HAVE_BMI2_INSTRUCTIONS)
        {
        using namespace base64::avx2;

        RUN_TEMPLATE1("avx2_bmi2/1", decode_bmi2, lookup_base);
        RUN_TEMPLATE1("avx2_bmi2/2", decode_bmi2, lookup_byte_blend);
        }
#   endif // HAVE_BMI2_INSTRUCTIONS
#endif // HAVE_AVX2_INSTRUCTIONS

#if defined(HAVE_AVX512_INSTRUCTIONS)
        {
        using namespace base64::avx512;

        RUN_TEMPLATE2("avx512/1", decode,               lookup_gather,      pack_identity);
        RUN_TEMPLATE2("avx512/2", decode,               lookup_comparisons, pack_improved);
        RUN_TEMPLATE2("avx512/3", decode_store_scatter, lookup_comparisons, pack_improved);
        }
#endif // HAVE_AVX512_INSTRUCTIONS

#if defined(HAVE_AVX512BW_INSTRUCTIONS)
        {
        using namespace base64::avx512bw;

        RUN_TEMPLATE2("avx512bw/1", decode, lookup_pshufb_bitmask, pack_madd);
        RUN_TEMPLATE2("avx512bw/2", decode, lookup_aqrit, pack_madd);
        }
#endif // HAVE_AVX512VBMI_INSTRUCTIONS

#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
        {
        using namespace base64::avx512vbmi;

        RUN_TEMPLATE2("avx512vbmi", decode, lookup, base64::avx512bw::pack_madd);
        }
#endif // HAVE_AVX512VBMI_INSTRUCTIONS

#if defined(HAVE_NEON_INSTRUCTIONS)
        {
        using namespace base64::neon;

        RUN_TEMPLATE1("neon/1", decode, lookup_byte_blend);
        RUN_TEMPLATE1("neon/2", decode, lookup_pshufb_bitmask);
        }
#endif // HAVE_NEON_INSTRUCTIONS

#if defined(HAVE_RVV_INSTRUCTIONS)
        {
        using namespace base64::rvv;

        RUN_TEMPLATE2("rvv/1", decode_vlen16_m8, lookup_vlen16_m8, pack_vlen16_m8);
        RUN_TEMPLATE2("rvv/2", decode_vlen16_m8, lookup_vlen16_m8, pack_vlen16_m8_ver2);
        RUN_TEMPLATE2("rvv/3", decode_vlen16_m8_omit_ws, lookup_vlen16_m8_omit_ws, pack_vlen16_m8);
        RUN_TEMPLATE2("rvv/4", decode_vlen16_m8_omit_ws, lookup_vlen16_m8_omit_ws, pack_vlen16_m8_ver2);
        }
#endif // HAVE_RVV_INSTRUCTIONS

#undef RUN_TEMPLATE1
#undef RUN_TEMPLATE2
    }

    virtual bool can_run(const std::string& name) const {
        return cmd.empty()
            || cmd.has(name);
    }

    template <typename FUNCTION>
    void run_function(const char* name, FUNCTION function) {
        if (!can_run(name))
            return;

        static_cast<Derived*>(this)->run_function_impl(name, function);
    }
};
