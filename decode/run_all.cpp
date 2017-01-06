/*
    This file is intended to be included inside a function.

    You have to define macros:

    RUN
    RUN_SSE_TEMPLATE1
    RUN_SSE_TEMPLATE2
    RUN_AVX2_TEMPLATE1
    RUN_AVX2_TEMPLATE2

*/

RUN("improved", base64::scalar::decode_lookup2);
RUN("scalar", base64::scalar::decode_lookup1);

#if defined(HAVE_BMI2_INSTRUCTIONS)
    RUN("scalar_bmi2", base64::scalar::decode_lookup1_bmi2);
#endif

#if defined(HAVE_SSE_INSTRUCTIONS)
{
    using namespace base64::sse;

    RUN_SSE_TEMPLATE2("sse/1",          decode, lookup_base,        pack_naive);
    RUN_SSE_TEMPLATE2("sse/2",          decode, lookup_byte_blend,  pack_naive);
    RUN_SSE_TEMPLATE2("sse/3",          decode, lookup_incremental, pack_naive);
    RUN_SSE_TEMPLATE2("sse/4",          decode, lookup_pshufb,      pack_naive);

    RUN_SSE_TEMPLATE2("sse/5",          decode, lookup_base,        pack_madd);
    RUN_SSE_TEMPLATE2("sse/6",          decode, lookup_byte_blend,  pack_madd);
    RUN_SSE_TEMPLATE2("sse/7",          decode, lookup_incremental, pack_madd);
    RUN_SSE_TEMPLATE2("sse/8",          decode, lookup_pshufb,      pack_madd);
    RUN_SSE_TEMPLATE2("sse/9",          decode, lookup_pshufb_bitmask, pack_madd);
}
#endif

#if defined(HAVE_XOP_INSTRUCTIONS)
    RUN_SSE_TEMPLATE2("xop", base64::sse::decode, base64::xop::lookup_pshufb_bitmask, base64::sse::pack_madd);
#endif

#if defined(HAVE_BMI2_INSTRUCTIONS)
    {
    using namespace base64::sse;

    RUN_SSE_TEMPLATE1("sse_bmi2/1", decode_bmi2, lookup_base);
    RUN_SSE_TEMPLATE1("sse_bmi2/2", decode_bmi2, lookup_byte_blend);
    RUN_SSE_TEMPLATE1("sse_bmi2/3", decode_bmi2, lookup_incremental);
    }
#endif

#if defined(HAVE_AVX2_INSTRUCTIONS)
    {
    using namespace base64::avx2;

    RUN_AVX2_TEMPLATE2("avx2/1",    decode, lookup_base,        pack_naive);
    RUN_AVX2_TEMPLATE2("avx2/2",    decode, lookup_byte_blend,  pack_naive);
    RUN_AVX2_TEMPLATE2("avx2/3",    decode, lookup_pshufb,      pack_naive);

    RUN_AVX2_TEMPLATE2("avx2/4",    decode, lookup_base,        pack_madd);
    RUN_AVX2_TEMPLATE2("avx2/5",    decode, lookup_byte_blend,  pack_madd);
    RUN_AVX2_TEMPLATE2("avx2/6",    decode, lookup_pshufb,      pack_madd);
    RUN_AVX2_TEMPLATE2("avx2/7",    decode, lookup_pshufb_bitmask, pack_madd);
    }

    #if defined(HAVE_BMI2_INSTRUCTIONS)
        {
        using namespace base64::avx2;

        RUN_AVX2_TEMPLATE1("avx2_bmi2/1", decode_bmi2, lookup_base);
        RUN_AVX2_TEMPLATE1("avx2_bmi2/2", decode_bmi2, lookup_byte_blend);
        }
    #endif // HAVE_BMI2_INSTRUCTIONS
#endif // HAVE_AVX2_INSTRUCTIONS

#if defined(HAVE_AVX512_INSTRUCTIONS)
    {
    using namespace base64::avx512;

    RUN_AVX2_TEMPLATE2("avx512/1", decode,               lookup_gather,      pack_identity);
    RUN_AVX2_TEMPLATE2("avx512/2", decode,               lookup_comparisons, pack_improved);
    RUN_AVX2_TEMPLATE2("avx512/3", decode_store_scatter, lookup_comparisons, pack_improved);
    }
#endif // HAVE_AVX512_INSTRUCTIONS

#if defined(HAVE_AVX512BW_INSTRUCTIONS)
    {
    using namespace base64::avx512bw;

    RUN_AVX2_TEMPLATE2("avx512bw", decode, lookup, pack_madd);
    }
#endif // HAVE_AVX512BW_INSTRUCTIONS

#if defined(HAVE_NEON_INSTRUCTIONS)
    {
    using namespace base64::neon;

    RUN_SSE_TEMPLATE1("neon/1", decode, lookup_byte_blend);
    }
#endif // HAVE_NEON_INSTRUCTIONS


