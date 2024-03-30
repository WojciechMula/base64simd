#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "../config.h"
#include "../benchmark.h"
#include "../memcpy.cpp"

#include "application.cpp"

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
        if((inalign !=0) || (outalign!=0)) {
            printf("warning: your data pointers are unaligned: %zu %zu\n", inalign, outalign);
        }
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

