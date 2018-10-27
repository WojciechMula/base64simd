#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "../config.h"
#include "../fnv32.cpp"

#include "application.cpp"

class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super;

    bool all_ok;
    uint32_t valid_hash;

public:
    Application(const CommandLine& c, const FunctionRegistry& r)
        : super(c, r)
        , all_ok(true)
        , valid_hash(0) {}

    bool run() {

        initialize();
        run_all();

        if (cmd.empty()) {
            puts(all_ok ? "all OK" : "!!!ERRORS!!!");
        }

        return all_ok;
    }

private:
    template<typename T>
    void run_function_impl(const char* name, T function) {

        printf("%*s ... ", -names.get_width(), names[name]);
        fflush(stdout);

        clear_output();
        function(input.get(), get_input_size(), output.get());

        const uint32_t hash = FNV32::get(reinterpret_cast<const char*>(output.get()), get_output_size());
        printf("hash: %08x", hash);
        if (valid_hash != 0) {
            const bool ok = (hash == valid_hash);
            printf(" %s", ok ? "OK" : "!!!ERROR!!!");
            all_ok = all_ok && ok;
        }

        putchar('\n');

        valid_hash = hash;
    }
};


int main(int argc, char* argv[]) {

    FunctionRegistry names;
    CommandLine cmd(argc, argv);
    Application app(cmd, names);

    return app.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}


