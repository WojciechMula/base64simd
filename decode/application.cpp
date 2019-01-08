#include "../cmdline.cpp"
#include "../benchmark.h"
#include "function_registry.cpp"
#include "all.cpp"
#include <cstring>

#if defined(HAVE_AVX512_INSTRUCTIONS)
#include <immintrin.h>
void* avx512_memcpy(void *dst, const void * src, size_t n) {
  if(n >= 64) {
    size_t nminus64 = n - 64;
    for(size_t i = 0; i <= nminus64; i+=64) {
      __m512i x = _mm512_loadu_si512((const char*)src + i);
      _mm512_storeu_si512((char*)dst + i, x);
    }
    size_t leftover = n % 64;
    ::memcpy((char*)dst + n - leftover, (const char*)src + n - leftover, leftover);
    return dst;
  } else {
    return ::memcpy(dst,src,n);
  }
}
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
        , count(64*1024*1024)
        , iterations(10)
        , initialized(false)
        , quiet(false) {}

protected:
    void initialize() {

        if (initialized) {
            return;
        }

        base64::scalar::prepare_lookup();
        base64::scalar::prepare_lookup32();
#if defined(HAVE_AVX512_INSTRUCTIONS)
        base64::avx512::prepare_lookups();
#endif // HAVE_AVX512_INSTRUCTIONS
#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
        base64::avx512vbmi::prepare_lookups();
#endif // HAVE_AVX512VBMI_INSTRUCTIONS
        input.reset (new uint8_t[get_input_size()]);
        output.reset(new uint8_t[get_output_size() + 64]);

        if (!quiet) {
            printf("input size: %lu\n", get_input_size());
            printf("number of iterations: %u\n", iterations);
            printf("We report the time in cycles per output byte.\n");
            printf("For reference, we present the time needed to copy %zu bytes.\n", get_output_size());
        }

        fill_input();
        BEST_TIME(/**/, ::memcpy(output.get(),input.get(),get_output_size()), "memcpy", iterations, get_output_size());

#if defined(HAVE_AVX512_INSTRUCTIONS)
        size_t inalign = reinterpret_cast<uintptr_t>(input.get()) & 63;
        size_t outalign = reinterpret_cast<uintptr_t>(output.get()) & 63;
        if((inalign !=0) || (outalign!=0))
          printf("warning: your data pointers are unaligned: %zu %zu\n", inalign, outalign);
        BEST_TIME(/**/, avx512_memcpy(output.get(),input.get(),get_output_size()), "memcpy (avx512)", iterations, get_output_size());
#endif
        initialized = true;
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
