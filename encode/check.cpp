#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <cstdlib>

#include "../ansi.cpp"
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

        if (all_ok) {
            printf("%sAll OK%s\n", ansi::str::GREEN, ansi::str::RESET);
        }

        return all_ok;
    }

private:
    template<typename T>
    uint32_t run_function_impl(const char* name, T callback) {

        printf("%-50s... ", name);
        fflush(stdout);

        clear_output();
        callback(input.get(), get_input_size(), output.get());

        const uint32_t hash = FNV32::get(reinterpret_cast<const char*>(output.get()), get_output_size() - 16);

        printf("%08x", hash);
        if (reference != 0) {
            const bool err = (reference != hash);
            if (err) {
                printf(" %sERROR%s", ansi::str::RED, ansi::str::RESET);
                all_ok = false;
            } else {
                printf(" %sOK%s", ansi::str::GREEN, ansi::str::RESET);
            }
        } else {
            reference = hash;
        }

        putchar('\n');

        return hash;
    }

    virtual bool can_run(const std::string&, const std::string&) const override {
        return true;
    }
};


int main(int argc, char* argv[]) {
    CommandLine cmd(argc, argv);
    Application app(cmd);

    return app.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}

