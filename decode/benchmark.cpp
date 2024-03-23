#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "../config.h"
#include "../benchmark.h"

#include "application.cpp"

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

#ifdef HAVE_RVV_INSTRUCTIONS
void* rvv_memcpy(void* pdst, const void* psrc, size_t n) {
    uint8_t* dst = reinterpret_cast<uint8_t*>(pdst);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(psrc);

    while (n > 0) {
        // That's a really nice use-case for RVV.
        const size_t     vl = __riscv_vsetvl_e8m8(n);
        const vuint8m8_t v0 = __riscv_vle8_v_u8m8(src, vl);
        __riscv_vse8_v_u8m8(dst, v0, vl);

        n -= vl;
        src += vl;
        dst += vl;
    }

    return reinterpret_cast<void*>(dst);
}
#endif // HAVE_RVV_INSTRUCTIONS

#ifndef BUFFER_SIZE
#   define BUFFER_SIZE (4 * 1024)
#endif

#ifndef ITERATIONS
#   define ITERATIONS (10000)
#endif

class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super;

public:
    Application(const CommandLine& c, const FunctionRegistry& r) : super(c, r) {
        /* override the defaults */
        count      = BUFFER_SIZE;
        iterations = ITERATIONS;
    }

    void run() {
        initialize();

        run_all();
    }

private:
    virtual void custom_initialize() override {
        if (!quiet) {
            printf("input size: %lu\n", get_input_size());
            printf("number of iterations: %u\n", iterations);
            printf("We report the time in cycles per output byte.\n");
            printf("For reference, we present the time needed to copy %zu bytes.\n", get_output_size());
        }

        BEST_TIME(/**/, ::memcpy(output.get(),input.get(),get_output_size()), "memcpy", iterations, get_output_size());

#if defined(HAVE_AVX512_INSTRUCTIONS)
        size_t inalign = reinterpret_cast<uintptr_t>(input.get()) & 63;
        size_t outalign = reinterpret_cast<uintptr_t>(output.get()) & 63;
        if((inalign !=0) || (outalign!=0))
          printf("warning: your data pointers are unaligned: %zu %zu\n", inalign, outalign);
        BEST_TIME(/**/, avx512_memcpy(output.get(), input.get(), get_output_size()), "memcpy (avx512)", iterations, get_output_size());
#endif

#if defined(HAVE_RVV_INSTRUCTIONS)
        BEST_TIME(/**/, rvv_memcpy(output.get(), input.get(), get_output_size()), "memcpy (RVV)", iterations, get_output_size());
#endif
    }

    virtual bool can_run(const std::string&) const override {
        return true;
    }

    template<typename DECODE_FN>
    void run_function_impl(const char* name, DECODE_FN decode_fn) {

        auto fn = [this, decode_fn]() {
            decode_fn(input.get(), get_input_size(), output.get());
        };

        const char* description = names.get(name).formatted().c_str();

        BEST_TIME(/**/, fn(), description, iterations, count);
    }
};


int main(int argc, char* argv[]) {

    FunctionRegistry registry;
    CommandLine cmd(argc, argv);
    Application app(cmd, registry);

    app.run();

    return EXIT_SUCCESS;
}

