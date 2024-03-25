#include <vector>
#include <cstdlib>
#include <cstdint>

#include "../cmdline.cpp"
#include "../fnv32.cpp"

#include "decode.common.cpp"
#include "decode.scalar.cpp"

#include "initialize.cpp"

#ifndef BUFFER_SIZE
#   define BUFFER_SIZE (64*1024*1024)
#endif

#ifndef ITERATIONS
#   define ITERATIONS (10)
#endif

class Application {
protected:
    const CommandLine& cmd;
    unsigned count;
    unsigned iterations;
    bool initialized;
    bool quiet;
    size_t tests_run;
    size_t test_failures;
    uint32_t valid_hash;

    std::string input;
    std::string output;
public:
    Application(const CommandLine& c)
        : cmd(c)
        , count(BUFFER_SIZE)
        , iterations(ITERATIONS)
        , initialized(false)
        , quiet(false)
        , valid_hash(0) {}

protected:
    void initialize() {
        if (initialized) {
            return;
        }

        srand(42);
        fill_input();
        output.resize(get_output_size());

        initialized = true;
    }

    size_t get_input_size() const {
        return count;
    }

    size_t get_output_size() const {
        return (3*count)/4;
    }

    void clear_output() {
        output.resize(0);
        output.resize(get_output_size());
    }

    void fill_input() {
        int whitespaces = 0;
        const size_t n = get_input_size();
        while (input.size() < n) {
            if (whitespaces > 0) {
                input.push_back(' ');
                whitespaces -= 1;
            } else {
                if (rand() % 100 > 80) {
                    whitespaces = rand() % 80;
                } else {
                    const uint8_t idx = input.size() * 71;
                    input.push_back(base64::lookup[idx % 64]);
                }
            }
        }
    }

    template <typename FN_DECODE>
    void decode(const char* name, FN_DECODE function) {
        tests_run += 1;

        printf("%s ... ", name);
        fflush(stdout);

        clear_output();
        const uint8_t* decoded = function(
            reinterpret_cast<const uint8_t*>(input.data()),
            get_input_size(),
            (uint8_t*)(output.data()));

        const size_t decoded_size = decoded - reinterpret_cast<const uint8_t*>(output.data());

        const uint32_t hash = FNV32::get(reinterpret_cast<const char*>(output.data()), decoded_size);
        printf("hash: %08x", hash);
        if (valid_hash != 0) {
            const bool ok = (hash == valid_hash);
            printf(" %s", ok ? "OK" : "!!!ERROR!!!");
            if (!ok) {
                test_failures += 1;
            }
        }

        putchar('\n');

        valid_hash = hash;
    }

public:
    bool run() {
        initialize();

        decode("reference implementation", base64::scalar::decode_lookup_ws);

        if (test_failures > 0) {
            return false;
        }

        return true;
    }
};

int main(int argc, char* argv[]) {
    CommandLine cmd(argc, argv);
    Application app(cmd);

    return app.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
