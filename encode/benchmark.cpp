#include <cstdio>

#include "application.cpp"

#include "../benchmark.h"


class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super; 

public:
    Application(const CommandLine& cmdline)
        : super(cmdline, 1024, 10000) {}

    void run() {

        initialize();

        run_all();
    }

private:
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

