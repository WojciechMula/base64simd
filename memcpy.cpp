#if defined(HAVE_AVX512_INSTRUCTIONS)
void* avx512_memcpy(void *dst, const void * src, size_t n) {
  if(n >= 64) {
    size_t i = 0;
    if(n >= 4*64) {
      for(; i <= n - 4*64; i+=4*64) {
        __m512i x0 = _mm512_loadu_si512((const char*)src + i);
        __m512i x1 = _mm512_loadu_si512((const char*)src + i + 64);
        __m512i x2 = _mm512_loadu_si512((const char*)src + i + 128);
        __m512i x3 = _mm512_loadu_si512((const char*)src + i + 192);
        _mm512_storeu_si512((char*)dst + i, x0);
        _mm512_storeu_si512((char*)dst + i + 64, x1);
        _mm512_storeu_si512((char*)dst + i + 128, x2);
        _mm512_storeu_si512((char*)dst + i + 192, x3);
     }
    }
    if(n>=64) {
      for(; i <= n - 64; i+=64) {
        __m512i x0 = _mm512_loadu_si512((const char*)src + i);
        _mm512_storeu_si512((char*)dst + i, x0);
      }
    }
    size_t leftover = n % 64;
    ::memcpy((char*)dst + n - leftover, (const char*)src + n - leftover, leftover);
    return dst;
  } else {
    return ::memcpy(dst,src,n);
  }
  
}
#endif

#ifdef HAVE_RVV_INSTRUCTIONS
void* rvv_memcpy(void* pdst, const void* psrc, size_t n) {
    uint8_t* dst = reinterpret_cast<uint8_t*>(pdst);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(psrc);

    while (n > 0) {
        // That's a really nice use-case for RVV.
        const size_t     vl = __riscv_vsetvl_e8m8(n);
        const vuint8m8_t v0 = __riscv_vle8_v_u8m8(src, vl);
        __riscv_vse8_v_u8m8(dst, v0, vl);

        n -= vl;
        src += vl;
        dst += vl;
    }

    return reinterpret_cast<void*>(dst);
}
#endif // HAVE_RVV_INSTRUCTIONS
