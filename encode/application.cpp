#include "../config.h"
#include <cstdint>
#include <cstdlib>
#include <memory>

#include "encode.scalar.cpp"
#include "lookup.swar.cpp"
#include "encode.swar.cpp"
#if defined(HAVE_SSE_INSTRUCTIONS)
#   include "lookup.sse.cpp"
#   include "encode.sse.cpp"
#endif
#if defined(HAVE_XOP_INSTRUCTIONS)
#   include "encode.xop.cpp"
#   include "lookup.xop.cpp"
#endif
#if defined(HAVE_AVX2_INSTRUCTIONS)
#   include "lookup.avx2.cpp"
#   include "encode.avx2.cpp"
#endif
#if defined(HAVE_AVX512_INSTRUCTIONS)
#   include "../avx512_swar.cpp"
#   include "unpack.avx512.cpp"
#   include "lookup.avx512.cpp"
#   include "encode.avx512.cpp"
#endif
#if defined(HAVE_AVX512BW_INSTRUCTIONS)
#   include "encode.avx512bw.cpp"
#   include "lookup.avx512bw.cpp"
#   include "unpack.avx512bw.cpp"
#endif
#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
#   include "encode.avx512vbmi.cpp"
#endif
#if defined(HAVE_AVX512VL_INSTRUCTIONS)
#   include "encode.avx512vl.cpp"
#endif
#if defined(HAVE_NEON_INSTRUCTIONS)
#   include "lookup.neon.cpp"
#   include "encode.neon.cpp"
#endif

#include "../cmdline.cpp"


template <typename Derived>
class ApplicationBase {

protected:
    const CommandLine& cmd;
    const size_t count;
    const size_t iterations;
    bool initialized;

    std::unique_ptr<uint8_t> input;
    std::unique_ptr<uint8_t> output;
public:
    ApplicationBase(const CommandLine& c, size_t cnt, size_t iters)
        : cmd(c)
        , count(cnt)
        , iterations(iters)
        , initialized(false) {}

    void initialize() {

        if (initialized) {
            return;
        }

        input.reset (new uint8_t[get_input_size()]);
        output.reset(new uint8_t[get_output_size()]);

        printf("input size: %lu\n", get_input_size());

        fill_input();

        initialized = true;
    }

protected:
    size_t get_input_size() const {
        return 3*count;
    }

    size_t get_output_size() const {
        return 4*count;
    }

    void fill_input() {
        for (unsigned i=0; i < get_input_size(); i++) {
            input.get()[i] = i * 71;
        }
    }

    void clear_output() {
        memset(output.get(), 0, get_output_size());
    }

protected:
    void run_all() {
        
        if (can_run("scalar", "scalar32")) {
            run_function("scalar (32 bit)", base64::scalar::encode32);
        }

#if !defined(HAVE_NEON_INSTRUCTIONS)
        if (can_run("scalar", "scalar64")) {
            run_function("scalar (64 bit)", base64::scalar::encode64);
        }

        if (can_run("scalar", "swar")) {
            run_function("SWAR (64 bit)", base64::swar::encode);
        }
#endif

#if defined(HAVE_SSE_INSTRUCTIONS)
        if (can_run("sse", "sse")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_naive, input, bytes, output);
            };
            run_function("SSE (lookup: naive)", fun);
        }

        if (can_run("sse", "sse1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_version1, input, bytes, output);
            };
            run_function("SSE (lookup: other improved)", fun);
        }

        if (can_run("sse" ,"sse2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_version2, input, bytes, output);
            };
            run_function("SSE (lookup: improved)", fun);
        }

        if (can_run("sse", "sse3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_pshufb, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb-based)", fun);
        }

        if (can_run("sse", "sse3/improved")) {
            auto sse_pshufb_improved = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb improved)", sse_pshufb_improved);
        }

        if (can_run("sse", "sse1/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_unrolled(base64::sse::lookup_version1, input, bytes, output);
            };
            run_function("SSE (lookup: other improved, unrolled)", fun);
        }

        if (can_run("sse", "sse2/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_unrolled(base64::sse::lookup_version2, input, bytes, output);
            };
            run_function("SSE (lookup: improved, unrolled)", fun);
        }

        if (can_run("sse", "sse2/unrolled2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_unrolled(base64::sse::lookup_pshufb, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb-based, unrolled)", fun);
        }

        if (can_run("sse", "sse3/improved/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb improved unrolled)", fun);
        }

        if (can_run("sse", "sse2/fully_unrolled")) {
            run_function("SSE (fully unrolled improved lookup)", base64::sse::encode_full_unrolled);
        }
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("bmi", "bmi1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_bmi2(base64::sse::lookup_naive, input, bytes, output);
            };
            run_function("SSE & BMI2 (lookup: naive)", fun);
        }

        if (can_run("bmi", "bmi2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_bmi2(base64::sse::lookup_version1, input, bytes, output);
            };
            run_function("SSE & BMI2 (lookup: improved)", fun);
        }

        if (can_run("bmi", "bmi3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_bmi2(base64::sse::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("SSE & BMI2 (lookup: pshufb improved)", fun);
        }
#endif

#if defined(HAVE_XOP_INSTRUCTIONS)
        if (can_run("xop", "xop/vpermb")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::xop::encode(base64::xop::lookup, input, bytes, output);
            };
            run_function("XOP (vpermb)", fun);
        }

        if (can_run("xop", "xop/pshufb")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::xop::encode(base64::sse::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("XOP (pshufb improved)", fun);
        }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
        if (can_run("avx2", "avx2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode(base64::avx2::lookup_version2, input, bytes, output);
            };
            run_function("AVX2 (lookup: improved)", fun);
        }

        if (can_run("avx2", "avx2/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_unrolled(base64::avx2::lookup_version2, input, bytes, output);
            };
            run_function("AVX2 (lookup: improved, unrolled)", fun);
        }

        if (can_run("avx2", "avx2/pshufb")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode(base64::avx2::lookup_pshufb, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb-based)", fun);
        }

        if (can_run("avx2", "avx2/pshufb/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_unrolled(base64::avx2::lookup_pshufb, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb-based, unrolled)", fun);
        }

        if (can_run("avx2", "avx2/pshufb/improved")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode(base64::avx2::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb improved)", fun);
        }

        if (can_run("avx2", "avx2/pshufb/improved/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_unrolled(base64::avx2::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb unrolled improved)", fun);
        }

    #if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("avx2", "avx2/pshufb/improved/unrolled")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_bmi2(base64::avx2::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("AVX2 & BMI (lookup: pshufb improved)", fun);
        }
    #endif
#endif

#if defined(HAVE_AVX512_INSTRUCTIONS)
        if (can_run("avx512", "avx512/logic")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_incremental_logic, unpack, input, bytes, output);
            };
            run_function("AVX512 (incremental logic)", fun);
        }

        if (can_run("avx512", "avx512/logic/improved")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_incremental_logic_improved, unpack, input, bytes, output);
            };
            run_function("AVX512 (incremental logic improved)", fun);
        }

        if (can_run("avx512", "avx512/logic/improved/gather")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode_load_gather(lookup_incremental_logic_improved, unpack, input, bytes, output);
            };
            run_function("AVX512 (incremental logic improved with gather load)", fun);
        }

        if (can_run("avx512", "avx512/binsearch")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_binary_search, unpack, input, bytes, output);
            };
            run_function("AVX512 (binary search)", fun);
        }

        if (can_run("avx512", "avx512/gather")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_gather, unpack_identity, input, bytes, output);
            };
            run_function("AVX512 (gather)", fun);
        }
#endif

#if defined(HAVE_AVX512BW_INSTRUCTIONS)
        if (can_run("avx512bw", "avx512bw/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512bw;
                encode(lookup_version2, unpack, input, bytes, output);
            };
            run_function("AVX512BW (lookup: optimized2)", fun);
        }

        if (can_run("avx512bw", "avx512bw/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512bw;
                encode(lookup_pshufb_improved, unpack, input, bytes, output);
            };
            run_function("AVX512BW (lookup: pshufb improved)", fun);
        }

        if (can_run("avx512bw", "avx512bw/3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512bw;
                encode_unrolled2(lookup_pshufb_improved, unpack, input, bytes, output);
            };
            run_function("AVX512BW (lookup: pshufb improved, unrolled x 2)", fun);
        }
#endif

#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
        if (can_run("avx512vbmi", "avx512vbmi")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx512vbmi::encode(input, bytes, output);
            };
            run_function("AVX512VBMI", fun);
        }
#endif

#if defined(HAVE_AVX512VL_INSTRUCTIONS)
        if (can_run("avx512vl", "avx512vl")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx512vl::encode(input, bytes, output);
            };
            run_function("AVX512VL", fun);
        }
#endif

#if defined(HAVE_NEON_INSTRUCTIONS)
        if (can_run("neon", "neon/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::neon;
                encode(lookup_naive, input, bytes, output);
            };
            run_function("ARM NEON (naive lookup)", fun);
        }

        if (can_run("neon", "neon/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::neon;
                encode(lookup_version2, input, bytes, output);
            };
            run_function("ARM NEON (optimized lookup)", fun);
        }

        if (can_run("neon", "neon/3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::neon;
                encode(lookup_pshufb_improved, input, bytes, output);
            };
            run_function("ARM NEON (pshufb improved lookup)", fun);
        }
#endif
    }

    virtual bool can_run(const std::string& group_name, const std::string& function_name) const {
        return cmd.empty()
            || cmd.has(group_name)
            || cmd.has(function_name);
    }

    template<typename T>
    void run_function(const char* name, T callback) {
        static_cast<Derived*>(this)->run_function_impl(name, callback);
    }
};

