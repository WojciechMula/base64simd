#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "../config.h"
#include "../gettime.cpp"

#include "application.cpp"

class Application final: public ApplicationBase<Application> {

    using super = ApplicationBase<Application>;
    friend super;

    bool print_csv;
    double reference;

public:
    Application(const CommandLine& c, const FunctionRegistry& r)
        : ApplicationBase(c, r)
        , print_csv(cmd.has_flag("csv"))
        , reference(0.0) {

        quiet = print_csv;
    }

    void run() {

        initialize();
        
        run_all();
    }

private:
    template<typename T>
    double run_function_impl(const char* name, T callback) {

        if (print_csv) {
            const auto& fn = names.get(name);
            printf("%s, %s, %s", fn.display_name.c_str(), fn.lookup_method.c_str(), fn.pack_method.c_str());
        } else {
            printf("%*s ... ", -names.get_width(), names[name]);
        }

        fflush(stdout);

        unsigned n = iterations;
        double time = -1;
        while (n-- > 0) {

            const auto t1 = get_time();
            callback(input.get(), get_input_size(), output.get());
            const auto t2 = get_time();

            const double t = (t2 - t1)/1000000.0;
            if (time < 0) {
                time = t;
            } else {
                time = std::min(time, t);
            }
        }

        if (print_csv) {
            printf(", %0.5f\n", time);
        } else {
            if (reference > 0.0) {
                printf("%0.5f (speed-up: %0.2f)", time, reference/time);
            } else {
                printf("%0.5f", time);
            }
            putchar('\n');
        }


        return time;
    }
};

int main(int argc, char* argv[]) {

    FunctionRegistry registry;
    CommandLine cmd(argc, argv);
    Application app(cmd, registry);

    app.run();

    return EXIT_SUCCESS;
}

