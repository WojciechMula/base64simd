#include <unordered_map>


class Function {

    friend class FunctionRegistry;

public:
    Function(const std::string& n, const std::string& d, const std::string& l, const std::string& p)
        : name(n)
        , display_name(d)
        , lookup_method(l)
        , pack_method(p) {}

public:
    std::string  name;
    std::string  display_name;
    std::string  lookup_method;
    std::string  pack_method;

    const std::string& formatted() const { return image; }

private:
    std::string image;
    std::string render() const {
        if (lookup_method == "N/A" && pack_method == "N/A") {
            return display_name;
        } else {
            return display_name + " (lookup: " + lookup_method + ", pack: " + pack_method + ")";
        }
    }
};


class FunctionRegistry final {

    std::unordered_map<std::string, Function> registry;
    int widest_image;

public:
    FunctionRegistry() {
        build();
    }

    const Function& get(const std::string& name) const {
        return registry.at(name);
    }

    const char* operator[](const std::string& name) const {
        return get(name).image.c_str();
    }

    int get_width() const {
        return widest_image;
    }

private:
    void build();
    void add(const std::string& name, const std::string& display_name, const std::string& lookup_method, const std::string& pack_method);
    void add(const std::string& name, const std::string& display_name);
};


void FunctionRegistry::build() {

    add("improved",         "improved scalar");
    add("scalar",           "scalar");
#if defined(HAVE_BMI2_INSTRUCTIONS)
    add("scalar_bmi2",      "scalar & BMI2");
#endif
    add("sse/1", "SSE", "base",         "naive");
    add("sse/2", "SSE", "byte blend",   "naive");
    add("sse/3", "SSE", "incremental",  "naive");
    add("sse/4", "SSE", "pshufb",       "naive");

    add("sse/5", "SSE", "base",             "multiply-add");
    add("sse/6", "SSE", "byte blend",       "multiply-add");
    add("sse/7", "SSE", "incremental",      "multiply-add");
    add("sse/8", "SSE", "pshufb",           "multiply-add");
    add("sse/9", "SSE", "pshufb bitmask",   "multiply-add");
    add("sse/10", "SSE (aqrit)");

#if defined(HAVE_XOP_INSTRUCTIONS)
    add("xop", "XOP", "pshufb bitmask", "multiply-add");
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
    add("sse_bmi2/1", "SSE & BMI2", "base",         "N/A");
    add("sse_bmi2/2", "SSE & BMI2", "byte blend",   "N/A");
    add("sse_bmi2/3", "SSE & BMI2", "incremental",  "N/A");
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
    add("avx2/1", "AVX2", "base",         "naive");
    add("avx2/2", "AVX2", "byte blend",   "naive");
    add("avx2/3", "AVX2", "pshufb",       "naive");

    add("avx2/4", "AVX2", "base",         "multiply-add");
    add("avx2/5", "AVX2", "byte blend",   "multiply-add");
    add("avx2/6", "AVX2", "pshufb",       "multiply-add");
    add("avx2/7", "AVX2", "pshufb bitmask", "multiply-add");

# if defined(HAVE_BMI2_INSTRUCTIONS)
    add("avx2_bmi2/1", "AVX2 & BMI2", "base",       "N/A");
    add("avx2_bmi2/2", "AVX2 & BMI2", "byte blend", "N/A");
#   endif
#endif
#if defined(HAVE_AVX512BW_INSTRUCTIONS)
    add("avx512bw/1", "AVX512BW", "N/A",   "multiply-add");
    add("avx512bw/2", "AVX512BW", "aqrit", "multiply-add");
#endif
#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
    add("avx512vbmi", "AVX512VBMI", "N/A", "multiply-add");
#endif
#if defined(HAVE_AVX512_INSTRUCTIONS)
    add("avx512/1", "AVX512 (gather)");
    add("avx512/2", "AVX512 (store: vectorized)", "vectorized", "improved");
    add("avx512/3", "AVX512 (store: scatter)",    "vectorized", "improved");
#endif
#if defined(HAVE_NEON_INSTRUCTIONS)
    add("neon/1", "ARM NEON", "byte blend", "N/A");
    add("neon/2", "ARM NEON", "pshufb bitmask", "N/A");
#endif
    widest_image = 0;
    for (auto& it: registry) {

        it.second.image = it.second.render();
        widest_image = std::max(widest_image, static_cast<int>(it.second.image.size()));
    }
}


void FunctionRegistry::add(const std::string& name, const std::string& display_name, const std::string& lookup, const std::string& pack) {

    registry.insert({name, {name, display_name, lookup, pack}});
}

void FunctionRegistry::add(const std::string& name, const std::string& display_name) {

    registry.insert({name, {name, display_name, "N/A", "N/A"}});
}
