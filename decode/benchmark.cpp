#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "../config.h"
#include "../benchmark.h"

#include "application.cpp"

class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super;

public:
    Application(const CommandLine& c, const FunctionRegistry& r) : super(c, r) {

        count      = 4 * 1024; /* override the defaults */
        iterations = 1000;
    }

    void run() {

        initialize();

        run_all();
    }

private:
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

