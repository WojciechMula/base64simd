#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "../gettime.cpp"
#include "../fnv32.cpp"

#include "application.cpp"

#if !defined(BUFFER_SIZE)
#   define BUFFER_SIZE (64*1024*1024)
#endif

class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super;

    uint32_t reference;
    bool all_ok;

public:
    Application(const CommandLine& cmdline)
        : super(cmdline, BUFFER_SIZE, 1)
        , reference(0)
        , all_ok(true) {}

    bool run() {

        initialize();

        run_all();

        return all_ok;
    }

private:
    template<typename T>
    uint32_t run_function_impl(const char* name, T callback) {

        printf("%-40s... ", name);
        fflush(stdout);

        clear_output();
        callback(input.get(), get_input_size(), output.get());

        const uint32_t result = FNV32::get(reinterpret_cast<const char*>(output.get()), get_output_size() - 16);

        if (reference != 0) {
            printf("%08x %s\n", result, (reference != result) ? "!!!ERROR!!!" : "OK");
            all_ok = false;
        } else {
            printf("%08x\n", result);
        }

        return result;
    }

    virtual bool can_run(const std::string&, const std::string&) const override {
        return true;
    }
};


int main(int argc, char* argv[]) {

#if defined(HAVE_AVX512_INSTRUCTIONS)
    base64::avx512::initialize();
#endif

    CommandLine cmd(argc, argv);
    Application app(cmd);

    return app.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}

