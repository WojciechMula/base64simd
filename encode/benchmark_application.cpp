#include "../cmdline.cpp"

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
#if defined(HAVE_AVX512BMI_INSTRUCTIONS)
#   include "lookup.avx512vbmi.cpp"
#endif
#if defined(HAVE_NEON_INSTRUCTIONS)
#   include "lookup.neon.cpp"
#   include "encode.neon.cpp"
#endif

#include "application.cpp"

template <typename BenchmarkClass>
class BenchmarkApplication: public ApplicationBase {

public:
    BenchmarkApplication(const CommandLine& cmdline, size_t count, size_t iterations)
        : ApplicationBase(cmdline, count, iterations) {}

    BenchmarkApplication(const CommandLine& c)
#if defined(BUFFER_SIZE)
        : ApplicationBase(c, BUFFER_SIZE) {}
#else
        : ApplicationBase(c) {}
#endif

protected:
    int run_all() {
        
        #include "functions.cpp"

        if (can_run("scalar", "scalar32")) {
            measure("scalar (32 bit)", base64::scalar::encode32);
        }

#if !defined(HAVE_NEON_INSTRUCTIONS)
        if (can_run("scalar", "scalar64")) {
            measure("scalar (64 bit)", base64::scalar::encode64);
        }

        if (can_run("scalar", "swar")) {
            measure("SWAR (64 bit)", base64::swar::encode);
        }
#endif

#if defined(HAVE_SSE_INSTRUCTIONS)
        if (can_run("sse", "sse")) {
            measure("SSE (lookup: naive)", sse_naive);
        }

        if (can_run("sse", "sse1")) {
            measure("SSE (lookup: other improved)", sse_optimized1);
        }

        if (can_run("sse" ,"sse2")) {
            measure("SSE (lookup: improved)", sse_optimized2);
        }

        if (can_run("sse", "sse3")) {
            measure("SSE (lookup: pshufb-based)", sse_pshufb);
        }

        if (can_run("sse", "sse3/improved")) {
            measure("SSE (lookup: pshufb improved)", sse_pshufb_improved);
        }

        if (can_run("sse", "sse1/unrolled")) {
            measure("SSE (lookup: other improved, unrolled)", sse_optimized1_unrolled);
        }

        if (can_run("sse", "sse2/unrolled")) {
            measure("SSE (lookup: improved, unrolled)", sse_optimized2_unrolled);
        }

        if (can_run("sse", "sse2/unrolled2")) {
            measure("SSE (lookup: pshufb-based, unrolled)", sse_pshufb_unrolled);
        }

        if (can_run("sse", "sse3/improved/unrolled")) {
            measure("SSE (lookup: pshufb improved unrolled)", sse_pshufb_improved_unrolled);
        }

        if (can_run("sse", "sse2/fully_unrolled")) {
            measure("SSE (fully unrolled improved lookup)", base64::sse::encode_full_unrolled);
        }
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("bmi", "bmi1")) {
            measure("SSE & BMI2 (lookup: naive)", sse_bmi2_naive);
        }

        if (can_run("bmi", "bmi2")) {
            measure("SSE & BMI2 (lookup: improved)", sse_bmi2_optimized);
        }

        if (can_run("bmi", "bmi3")) {
            measure("SSE & BMI2 (lookup: pshufb improved)", sse_bmi2_pshufb_improved);
        }
#endif

#if defined(HAVE_XOP_INSTRUCTIONS)
        if (can_run("xop", "xop/vpermb")) {
            measure("XOP (vpermb)", xop_vperm);
        }

        if (can_run("xop", "xop/pshufb")) {
            measure("XOP (pshufb improved)", xop_pshufb);
        }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
        if (can_run("avx2", "avx2")) {
            measure("AVX2 (lookup: improved)", avx2_optimized2);
        }

        if (can_run("avx2", "avx2/unrolled")) {
            measure("AVX2 (lookup: improved, unrolled)", avx2_optimized2_unrolled);
        }

        if (can_run("avx2", "avx2/pshufb")) {
            measure("AVX2 (lookup: pshufb-based)", avx2_pshufb);
        }

        if (can_run("avx2", "avx2/pshufb/unrolled")) {
            measure("AVX2 (lookup: pshufb-based, unrolled)", avx2_pshufb_unrolled);
        }

        if (can_run("avx2", "avx2/pshufb/improved")) {
            measure("AVX2 (lookup: pshufb improved)", avx2_pshufb_improved);
        }

        if (can_run("avx2", "avx2/pshufb/improved/unrolled")) {
            measure("AVX2 (lookup: pshufb unrolled improved)", avx2_pshufb_improved_unrolled);
        }

    #if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("avx2", "avx2/pshufb/improved/unrolled")) {
            measure("AVX2 & BMI (lookup: pshufb improved)", avx2_bmi2_pshufb_improved);
        }
    #endif
#endif

#if defined(HAVE_AVX512_INSTRUCTIONS)
        if (can_run("avx512", "avx512/logic")) {
            measure("AVX512 (incremental logic)", avx512_swar_logic);
        }

        if (can_run("avx512", "avx512/logic/improved")) {
            measure("AVX512 (incremental logic improved)", avx512_swar_logic_improved);
        }

        if (can_run("avx512", "avx512/logic/improved/gather")) {
            measure("AVX512 (incremental logic improved with gather load)", avx512_swar_logic_improved_load_gather);
        }

        if (can_run("avx512", "avx512/binsearch")) {
            measure("AVX512 (binary search)", avx512_bin_search);
        }

        if (can_run("avx512", "avx512/gather")) {
            measure("AVX512 (gather)", avx512_gathers);
        }
#endif

#if defined(HAVE_AVX512BW_INSTRUCTIONS)
        if (can_run("avx512bw", "avx512bw/optimized2")) {
            measure("AVX512BW (lookup: optimized2)", avx512bw_optimized2);
        }

        if (can_run("avx512bw", "avx512bw/pshufb_improved")) {
            measure("AVX512BW (lookup: pshufb improved)", avx512bw_pshufb_improved);
        }
#endif

#if defined(HAVE_NEON_INSTRUCTIONS)
        if (can_run("neon", "neon/1")) {
            measure("ARM NEON (naive lookup)", neon_naive);
        }

        if (can_run("neon", "neon/2")) {
            measure("ARM NEON (optimized lookup)", neon_optimized);
        }

        if (can_run("neon", "neon/3")) {
            measure("ARM NEON (pshufb improved lookup)", neon_pshufb_improved);
        }
#endif

        return 0;
    }

    bool can_run(const std::string& group_name, const std::string& function_name) const {
        return cmd.empty()
            || cmd.has(group_name)
            || cmd.has(function_name);
    }

    template<typename T>
    void measure(const char* name, T callback) {
        static_cast<BenchmarkClass*>(this)->measure_impl(name, callback);
    }
};

