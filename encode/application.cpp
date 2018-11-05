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
        
        #include "functions.cpp"

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
            run_function("SSE (lookup: naive)", sse_naive);
        }

        if (can_run("sse", "sse1")) {
            run_function("SSE (lookup: other improved)", sse_optimized1);
        }

        if (can_run("sse" ,"sse2")) {
            run_function("SSE (lookup: improved)", sse_optimized2);
        }

        if (can_run("sse", "sse3")) {
            run_function("SSE (lookup: pshufb-based)", sse_pshufb);
        }

        if (can_run("sse", "sse3/improved")) {
            run_function("SSE (lookup: pshufb improved)", sse_pshufb_improved);
        }

        if (can_run("sse", "sse1/unrolled")) {
            run_function("SSE (lookup: other improved, unrolled)", sse_optimized1_unrolled);
        }

        if (can_run("sse", "sse2/unrolled")) {
            run_function("SSE (lookup: improved, unrolled)", sse_optimized2_unrolled);
        }

        if (can_run("sse", "sse2/unrolled2")) {
            run_function("SSE (lookup: pshufb-based, unrolled)", sse_pshufb_unrolled);
        }

        if (can_run("sse", "sse3/improved/unrolled")) {
            run_function("SSE (lookup: pshufb improved unrolled)", sse_pshufb_improved_unrolled);
        }

        if (can_run("sse", "sse2/fully_unrolled")) {
            run_function("SSE (fully unrolled improved lookup)", base64::sse::encode_full_unrolled);
        }
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("bmi", "bmi1")) {
            run_function("SSE & BMI2 (lookup: naive)", sse_bmi2_naive);
        }

        if (can_run("bmi", "bmi2")) {
            run_function("SSE & BMI2 (lookup: improved)", sse_bmi2_optimized);
        }

        if (can_run("bmi", "bmi3")) {
            run_function("SSE & BMI2 (lookup: pshufb improved)", sse_bmi2_pshufb_improved);
        }
#endif

#if defined(HAVE_XOP_INSTRUCTIONS)
        if (can_run("xop", "xop/vpermb")) {
            run_function("XOP (vpermb)", xop_vperm);
        }

        if (can_run("xop", "xop/pshufb")) {
            run_function("XOP (pshufb improved)", xop_pshufb);
        }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
        if (can_run("avx2", "avx2")) {
            run_function("AVX2 (lookup: improved)", avx2_optimized2);
        }

        if (can_run("avx2", "avx2/unrolled")) {
            run_function("AVX2 (lookup: improved, unrolled)", avx2_optimized2_unrolled);
        }

        if (can_run("avx2", "avx2/pshufb")) {
            run_function("AVX2 (lookup: pshufb-based)", avx2_pshufb);
        }

        if (can_run("avx2", "avx2/pshufb/unrolled")) {
            run_function("AVX2 (lookup: pshufb-based, unrolled)", avx2_pshufb_unrolled);
        }

        if (can_run("avx2", "avx2/pshufb/improved")) {
            run_function("AVX2 (lookup: pshufb improved)", avx2_pshufb_improved);
        }

        if (can_run("avx2", "avx2/pshufb/improved/unrolled")) {
            run_function("AVX2 (lookup: pshufb unrolled improved)", avx2_pshufb_improved_unrolled);
        }

    #if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("avx2", "avx2/pshufb/improved/unrolled")) {
            run_function("AVX2 & BMI (lookup: pshufb improved)", avx2_bmi2_pshufb_improved);
        }
    #endif
#endif

#if defined(HAVE_AVX512_INSTRUCTIONS)
        if (can_run("avx512", "avx512/logic")) {
            run_function("AVX512 (incremental logic)", avx512_swar_logic);
        }

        if (can_run("avx512", "avx512/logic/improved")) {
            run_function("AVX512 (incremental logic improved)", avx512_swar_logic_improved);
        }

        if (can_run("avx512", "avx512/logic/improved/gather")) {
            run_function("AVX512 (incremental logic improved with gather load)", avx512_swar_logic_improved_load_gather);
        }

        if (can_run("avx512", "avx512/binsearch")) {
            run_function("AVX512 (binary search)", avx512_bin_search);
        }

        if (can_run("avx512", "avx512/gather")) {
            run_function("AVX512 (gather)", avx512_gathers);
        }
#endif

#if defined(HAVE_AVX512BW_INSTRUCTIONS)
        if (can_run("avx512bw", "avx512bw/1")) {
            run_function("AVX512BW (lookup: optimized2)", avx512bw_optimized2);
        }

        if (can_run("avx512bw", "avx512bw/2")) {
            run_function("AVX512BW (lookup: pshufb improved)", avx512bw_pshufb_improved);
        }

        if (can_run("avx512bw", "avx512bw/3")) {
            run_function("AVX512BW (lookup: pshufb improved, unrolled x 2)", avx512bw_pshufb_improved_unrolled2);
        }
#endif

#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
        if (can_run("avx512vbmi", "avx512vbmi")) {
            run_function("AVX512VBMI", avx512vbmi);
        }
#endif

#if defined(HAVE_AVX512VL_INSTRUCTIONS)
        if (can_run("avx512vl", "avx512vl")) {
            run_function("AVX512VL", avx512vl);
        }
#endif

#if defined(HAVE_NEON_INSTRUCTIONS)
        if (can_run("neon", "neon/1")) {
            run_function("ARM NEON (naive lookup)", neon_naive);
        }

        if (can_run("neon", "neon/2")) {
            run_function("ARM NEON (optimized lookup)", neon_optimized);
        }

        if (can_run("neon", "neon/3")) {
            run_function("ARM NEON (pshufb improved lookup)", neon_pshufb_improved);
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

