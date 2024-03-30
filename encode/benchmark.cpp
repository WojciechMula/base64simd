#include <cstdio>

#include "application.cpp"

#include "../benchmark.h"
#include "../memcpy.cpp"

#ifndef ITERATIONS
#   define ITERATIONS 1000
#endif

class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super; 

public:
    Application(const CommandLine& cmdline)
        : super(cmdline, 1024, ITERATIONS) {}

    void run() {
        initialize();

        run_all();
    }

private:
    virtual void custom_initialize() override {
        printf("We report the time in cycles per input byte.\n");
        printf("For reference, we present the time needed to copy %zu bytes.\n", get_input_size());

        BEST_TIME(/**/, ::memcpy(output.get(), input.get(), get_input_size()), "memcpy", iterations, get_input_size());
#if defined(HAVE_AVX512_INSTRUCTIONS)
        size_t inalign = reinterpret_cast<uintptr_t>(input.get()) & 63;
        size_t outalign = reinterpret_cast<uintptr_t>(output.get()) & 63;
        if((inalign !=0) || (outalign!=0))
          printf("warning: your data pointers are unaligned: %zu %zu\n", inalign, outalign);
        BEST_TIME(/**/, avx512_memcpy(output.get(),input.get(),get_input_size()), "memcpy (avx512)", iterations, get_input_size());
#endif

#if defined(HAVE_RVV_INSTRUCTIONS)
        BEST_TIME(/**/, rvv_memcpy(output.get(), input.get(), get_output_size()), "memcpy (RVV)", iterations, get_output_size());
#endif
    }

    template<typename T>
    void run_function_impl(const char* name, T function) {

        auto fn = [this, function]() {
            function(input.get(), get_input_size(), output.get());
        };

        BEST_TIME(, fn(), name, iterations, get_input_size());
    }
};


int main(int argc, char* argv[]) {

    CommandLine cmd(argc, argv);
    Application app(cmd);

    app.run();

    return EXIT_SUCCESS;
}

