void initialize() {
    base64::scalar::prepare_lookup();
    base64::scalar::prepare_lookup32();
#if defined(HAVE_AVX512_INSTRUCTIONS)
    base64::avx512::prepare_lookups();
#endif
#if defined(HAVE_AVX512VBMI_INSTRUCTIONS)
    base64::avx512vbmi::prepare_lookups();
#endif
#if defined(HAVE_RVV_INSTRUCTIONS)
    base64::rvv::lookup::initialize();
    base64::rvv::pack::initialize();
#endif
}
