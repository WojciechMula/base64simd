#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "config.h"
#include "../benchmark.h"
#include "../cmdline.cpp"

#include "all.cpp"

#include "function_registry.cpp"
#include "application.cpp"

class Application final: public ApplicationBase {

public:
    Application(const CommandLine& c, const FunctionRegistry& r) : ApplicationBase(c, r) {

        count      = 4 * 1024; /* override the defaults */
        iterations = 1000;
    }

    int run() {

#define RUN(name, function) \
        if (cmd.empty() || cmd.has(name)) { \
            measure(name, function); \
        }


#define RUN_TEMPLATE1(name, decode_fn, lookup_fn) \
        if (cmd.empty() || cmd.has(name)) { \
            auto function = [](const uint8_t* input, size_t size, uint8_t* output) { \
                return decode_fn(lookup_fn, input, size, output); \
            }; \
            measure(name, function); \
        }

#define RUN_TEMPLATE2(name, decode_fn, lookup_fn, pack_fn) \
        if (cmd.empty() || cmd.has(name)) { \
            auto function = [](const uint8_t* input, size_t size, uint8_t* output) { \
                return decode_fn(lookup_fn, pack_fn, input, size, output); \
            }; \
            measure(name, function); \
        }

#define RUN_SSE_TEMPLATE1 RUN_TEMPLATE1
#define RUN_AVX2_TEMPLATE1 RUN_TEMPLATE1
#define RUN_SSE_TEMPLATE2 RUN_TEMPLATE2
#define RUN_AVX2_TEMPLATE2 RUN_TEMPLATE2

        #include "run_all.cpp"

        return 0;
    }

private:
    template<typename T>
    void measure(const char* name, T callback) {

        initialize();

        auto fn = [this, callback]() {
            callback(input.get(), get_input_size(), output.get());
        };

        const char* description = names.get(name).formatted().c_str();

        BEST_TIME(/**/, fn(), description, iterations, count);
    }
};


int main(int argc, char* argv[]) {

    FunctionRegistry registry;
    CommandLine cmd(argc, argv);
    Application app(cmd, registry);

    return app.run();
}

