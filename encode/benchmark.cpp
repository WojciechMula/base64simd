#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "config.h"
#include "../benchmark.h"

#include "benchmark_application.cpp"

class Application final: public BenchmarkApplication<Application> {

    friend class BenchmarkApplication<Application>;

public:
    Application(const CommandLine& cmdline)
        : BenchmarkApplication<Application>(cmdline, 1024, 1000) {}

    int run() {

        initialize();

        return run_all();
    }

private:
    template<typename T>
    void measure_impl(const char* name, T function) {

        auto fn = [this, function]() {
            function(input.get(), get_input_size(), output.get());
        };

        BEST_TIME(/**/, fn(), name, iterations, count);
    }
};


int main(int argc, char* argv[]) {

    CommandLine cmd(argc, argv);
    Application app(cmd);

    return app.run();
}

