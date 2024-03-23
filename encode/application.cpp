#include "../config.h"
#include "../benchmark.h"
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <cstring>

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
#if defined(HAVE_RVV_INSTRUCTIONS)
#   include "lookup.rvv.cpp"
#   include "encode.rvv.cpp"
#endif

#include "../cmdline.cpp"

#if defined(HAVE_AVX512_INSTRUCTIONS)
#include <immintrin.h>
void* avx512_memcpy(void *dst, const void * src, size_t n) {
  if(n >= 64) {
    size_t i = 0;
    if(n >= 4*64) {
      for(; i <= n - 4*64; i+=4*64) {
        __m512i x0 = _mm512_loadu_si512((const char*)src + i);
        __m512i x1 = _mm512_loadu_si512((const char*)src + i + 64);
        __m512i x2 = _mm512_loadu_si512((const char*)src + i + 128);
        __m512i x3 = _mm512_loadu_si512((const char*)src + i + 192);
        _mm512_storeu_si512((char*)dst + i, x0);
        _mm512_storeu_si512((char*)dst + i + 64, x1);
        _mm512_storeu_si512((char*)dst + i + 128, x2);
        _mm512_storeu_si512((char*)dst + i + 192, x3);
     }
    }
    if(n>=64) {
      for(; i <= n - 64; i+=64) {
        __m512i x0 = _mm512_loadu_si512((const char*)src + i);
        _mm512_storeu_si512((char*)dst + i, x0);
      }
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

        printf("input size: %lu, output size: %lu\n", get_input_size(), get_output_size());
        printf("number of iterations: %lu\n", iterations);
        printf("We report the time in cycles per input byte.\n");
        fill_input();
        printf("For reference, we present the time needed to copy %zu bytes.\n", get_input_size());
        BEST_TIME(/**/, ::memcpy(output.get(), input.get(), get_input_size()), "memcpy", iterations, get_input_size());
#if defined(HAVE_AVX512_INSTRUCTIONS)
        size_t inalign = reinterpret_cast<uintptr_t>(input.get()) & 63;
        size_t outalign = reinterpret_cast<uintptr_t>(output.get()) & 63;
        if((inalign !=0) || (outalign!=0))
          printf("warning: your data pointers are unaligned: %zu %zu\n", inalign, outalign);
        BEST_TIME(/**/, avx512_memcpy(output.get(),input.get(),get_input_size()), "memcpy (avx512)", iterations, get_input_size());
#endif
        initialized = true;
    }

protected:
    size_t get_input_size() const {
        return 3*count;
    }

    size_t get_output_size() const {
        const size_t padding_log2 = 8;
        const size_t mask = (size_t(1) << padding_log2) - 1;
        return (4*count + mask) & ~mask;
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
        
        if (can_run("scalar", "scalar/1")) {
            run_function("scalar (32 bit)", base64::scalar::encode32);
        }

#if !defined(HAVE_NEON_INSTRUCTIONS)
        if (can_run("scalar", "scalar/2")) {
            run_function("scalar (64 bit)", base64::scalar::encode64);
        }

        if (can_run("scalar", "scalar/3")) {
            run_function("SWAR (64 bit)", base64::swar::encode);
        }
#endif

#if defined(HAVE_SSE_INSTRUCTIONS)
        if (can_run("sse", "sse/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_naive, input, bytes, output);
            };
            run_function("SSE (lookup: naive)", fun);
        }

        if (can_run("sse", "sse/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_version1, input, bytes, output);
            };
            run_function("SSE (lookup: other improved)", fun);
        }

        if (can_run("sse" ,"sse/3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_version2, input, bytes, output);
            };
            run_function("SSE (lookup: improved)", fun);
        }

        if (can_run("sse", "sse/4")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_pshufb, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb-based)", fun);
        }

        if (can_run("sse", "sse/5")) {
            auto sse_pshufb_improved = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb improved)", sse_pshufb_improved);
        }

        if (can_run("sse", "sse/6")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_unrolled(base64::sse::lookup_version1, input, bytes, output);
            };
            run_function("SSE (lookup: other improved, unrolled)", fun);
        }

        if (can_run("sse", "sse/7")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_unrolled(base64::sse::lookup_version2, input, bytes, output);
            };
            run_function("SSE (lookup: improved, unrolled)", fun);
        }

        if (can_run("sse", "sse/8")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_unrolled(base64::sse::lookup_pshufb, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb-based, unrolled)", fun);
        }

        if (can_run("sse", "sse/9")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode(base64::sse::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("SSE (lookup: pshufb improved unrolled)", fun);
        }

        if (can_run("sse", "sse/10")) {
            run_function("SSE (fully unrolled improved lookup)", base64::sse::encode_full_unrolled);
        }
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("bmi", "bmi/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_bmi2(base64::sse::lookup_naive, input, bytes, output);
            };
            run_function("SSE & BMI2 (lookup: naive)", fun);
        }

        if (can_run("bmi", "bmi/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::sse::encode_bmi2(base64::sse::lookup_version1, input, bytes, output);
            };
            run_function("SSE & BMI2 (lookup: improved)", fun);
        }

        if (can_run("bmi", "bmi/3")) {
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
        if (can_run("avx2", "avx2/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode(base64::avx2::lookup_version2, input, bytes, output);
            };
            run_function("AVX2 (lookup: improved)", fun);
        }

        if (can_run("avx2", "avx2/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_unrolled(base64::avx2::lookup_version2, input, bytes, output);
            };
            run_function("AVX2 (lookup: improved, unrolled)", fun);
        }

        if (can_run("avx2", "avx2/3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode(base64::avx2::lookup_pshufb, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb-based)", fun);
        }

        if (can_run("avx2", "avx2/4")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_unrolled(base64::avx2::lookup_pshufb, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb-based, unrolled)", fun);
        }

        if (can_run("avx2", "avx2/5")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode(base64::avx2::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb improved)", fun);
        }

        if (can_run("avx2", "avx2/6")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_unrolled(base64::avx2::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("AVX2 (lookup: pshufb unrolled improved)", fun);
        }

    #if defined(HAVE_BMI2_INSTRUCTIONS)
        if (can_run("avx2", "avx2/7")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                base64::avx2::encode_bmi2(base64::avx2::lookup_pshufb_improved, input, bytes, output);
            };
            run_function("AVX2 & BMI (lookup: pshufb improved)", fun);
        }
    #endif
#endif

#if defined(HAVE_AVX512_INSTRUCTIONS)
        if (can_run("avx512", "avx512/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_incremental_logic, unpack, input, bytes, output);
            };
            run_function("AVX512 (incremental logic)", fun);
        }

        if (can_run("avx512", "avx512/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_incremental_logic_improved, unpack, input, bytes, output);
            };
            run_function("AVX512 (incremental logic improved)", fun);
        }

        if (can_run("avx512", "avx512/3")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode_load_gather(lookup_incremental_logic_improved, unpack, input, bytes, output);
            };
            run_function("AVX512 (incremental logic improved with gather load)", fun);
        }

        if (can_run("avx512", "avx512/4")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::avx512;
                encode(lookup_binary_search, unpack, input, bytes, output);
            };
            run_function("AVX512 (binary search)", fun);
        }

        if (can_run("avx512", "avx512/5")) {
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

#if defined(HAVE_RVV_INSTRUCTIONS)
        if (can_run("rvv", "rvv/1")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::rvv;
                encode(lookup_pshufb_improved, input, bytes, output);
            };
            run_function("RISC-V RVV (LMUL=1)", fun);
        }

        if (can_run("rvv", "rvv/2")) {
            auto fun = [](uint8_t* input, size_t bytes, uint8_t* output) {
                using namespace base64::rvv;
                encode_m8(lookup_wide_gather, input, bytes, output);
            };
            run_function("RISC-V RVV (LMUL=8)", fun);
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

