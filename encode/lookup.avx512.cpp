namespace base64 {

    namespace avx512 {

#define packed_dword(x) _mm512_set1_epi32(x)
#define packed_byte_aux(x) packed_dword((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))
#define packed_byte(x) packed_byte_aux(uint32_t(uint8_t(x)))

        static const char* lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        static uint32_t lookup_0[64];
        static uint32_t lookup_1[64];
        static uint32_t lookup_2[64];
        static uint32_t lookup_3[64];

        void initialize() {
            for (int i=0; i < 64; i++) {
                const uint32_t val = lookup[i];

                lookup_0[i] = val;
                lookup_1[i] = val << 8;
                lookup_2[i] = val << 16;
                lookup_3[i] = val << 24;
            }
        }

        void dump(__m512i v) {
            static uint8_t buf[64];

            _mm512_storeu_si512(reinterpret_cast<__m512i*>(buf), v);
        
            for (int i=0; i < 64; i++) {
                printf("%02x ", buf[i]);
            }

            putchar('\n');
        }

        __m512i lookup_gather(const __m512i in) {

            // [????????|ccdddddd|bbbbCCCC|aaaaaaBB]
            //           ^^       ^^^^^^^^       ^^
            //           lo        lo  hi        hi

            // split bytes into separate vectors
            const uint8_t MERGE = 0xca;

            const __m512i a = _mm512_and_si512(_mm512_srli_epi32(in, 2),   packed_dword(0x3f));
            const __m512i d = _mm512_and_si512(_mm512_srli_epi32(in, 16),  packed_dword(0x3f));

            const __m512i b0 = _mm512_and_si512(_mm512_srli_epi32(in, 12), packed_dword(0x0f));
            const __m512i b1 = _mm512_slli_epi32(in, 4);
            const __m512i b  = _mm512_ternarylogic_epi32(packed_dword(0x30), b1, b0, MERGE);

            const __m512i c0 = _mm512_and_si512(_mm512_srli_epi32(in, 6),  packed_dword(0x3c));
            const __m512i c1 = _mm512_srli_epi32(in, 22);
            const __m512i c  = _mm512_ternarylogic_epi32(packed_dword(0x3), c1, c0, MERGE);


            // do lookup
            const __m512i r0 = _mm512_i32gather_epi32(a, (const int*)lookup_0, 4);
            const __m512i r1 = _mm512_i32gather_epi32(b, (const int*)lookup_1, 4);
            const __m512i r2 = _mm512_i32gather_epi32(c, (const int*)lookup_2, 4);
            const __m512i r3 = _mm512_i32gather_epi32(d, (const int*)lookup_3, 4);

            const uint8_t OR_ALL = 0xfe;

            return _mm512_or_si512(r0, _mm512_ternarylogic_epi32(r1, r2, r3, OR_ALL));
        }


        const uint8_t XOR_ALL = 0x96;


        __m512i lookup_incremental_logic(const __m512i in) {

            using namespace avx512f_swar;

            __m512i shift = packed_byte('A');
            __m512i c0, c1, c2, c3;
            const __m512i MSB = packed_byte(0x80);
            
            // shift ^= cmp(i >= 26) & 6;
            c0 = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 26));
            c0 = _mm512_and_si512(c0, packed_byte(6));

            // shift ^= cmp(i >= 52) & 187;
            c1 = _mm512_and_si512(_mm512_add_epi32(in, packed_byte(0x80 - 52)), MSB);
            const __m512i c1msb = c1;
            c1 = _mm512_sub_epi32(c1, _mm512_srli_epi32(c1, 7));
            c1 = _mm512_and_si512(c1, packed_byte(187 & 0x7f));

            // shift ^= cmp(i >= 62) & 17;
            c2 = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 62));
            c2 = _mm512_and_si512(c2, packed_byte(17));

            // shift ^= cmp(i >= 63) & 29;
            c3 = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 63));
            c3 = _mm512_and_si512(c3, packed_byte(29));

            shift = _mm512_ternarylogic_epi32(shift, c0, c1, XOR_ALL);
            shift = _mm512_ternarylogic_epi32(shift, c2, c3, XOR_ALL);

            // produce the result
            return _mm512_xor_si512(_mm512_add_epi32(in, shift), c1msb);
        }


        __m512i lookup_incremental_logic_improved(const __m512i in) {

            using namespace avx512f_swar;

            __m512i shift;
            __m512i c0, c1, c2, c3;
            const __m512i MSB = packed_byte(0x80);

            // Note: an expression like shift ^= cmp(...) & const
            //       might be expressed with ternary logic
            /*
                s m c | shift ^ (mask & constant)
                ------+--------------------------
                0 0 0 | 0
                0 0 1 | 0
                0 1 0 | 0
                0 1 1 | 1
                1 0 0 | 1
                1 0 1 | 1
                1 1 0 | 1
                1 1 1 | 0 -> 0x78
            */
            const uint8_t XOR_AND = 0x78;

            // shift ^= cmp(i >= 26) & 6;
            // shift ^= cmp(i >= 52) & 187;
            // shift ^= cmp(i >= 62) & 17;
            // shift ^= cmp(i >= 63) & 29;

            c0 = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 26));
            c1 = _mm512_and_si512(_mm512_add_epi32(in, packed_byte(0x80 - 52)), MSB);
            const __m512i c1msb = c1;
            c1 = _mm512_sub_epi32(c1, _mm512_srli_epi32(c1, 7));
            c2 = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 62));
            c3 = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 63));

            shift = _mm512_ternarylogic_epi32(packed_byte('A'), c0, packed_byte(6), XOR_AND);
            shift = _mm512_ternarylogic_epi32(shift, c1, packed_byte(187 & 0x7f), XOR_AND);
            shift = _mm512_ternarylogic_epi32(shift, c2, packed_byte(17), XOR_AND);
            shift = _mm512_ternarylogic_epi32(shift, c3, packed_byte(29), XOR_AND);

            // produce the result
            return _mm512_xor_si512(_mm512_add_epi32(in, shift), c1msb);
        }


        const uint8_t BIT_MERGE = 0xca;


        __m512i lookup_binary_search(const __m512i in) {

            using namespace avx512f_swar;

            __m512i cmp1_mask;
            __m512i shift;
            __m512i shift_step2;
            __m512i cmp2_mask;
            __m512i cmp2_value;
            __m512i cmp_63_mask;
            
            // cmp1_mask    = cmp(i >= 52)
            cmp1_mask = _mm512_cmpge_mask8bit(in, packed_byte(0x80 - 52));

            // shift        = bit_merge(cmp1_mask, '0' - 52, 'A')
            shift           = _mm512_ternarylogic_epi32(cmp1_mask, packed_byte('0' - 52), packed_byte('A'), BIT_MERGE);
            // cmp2_value   = bit_merge(cmp1_mask, 62, 26)
            cmp2_value     = _mm512_ternarylogic_epi32(cmp1_mask, packed_byte(0x80 - 62), packed_byte(0x80 - 26), BIT_MERGE);
            // shift_step2  = bit_merge(cmp1_mask, '+' - 62, 'a' - 26)
            shift_step2 = _mm512_ternarylogic_epi32(cmp1_mask, packed_byte('+' - 62), packed_byte('a' - 26), BIT_MERGE);


            // cmp2_mask    = cmp(i >= cmp_value)
            cmp2_mask = _mm512_cmpge_mask8bit(in, cmp2_value);

            // shift        = bit_merge(cmp2_mask, shift, shift_step2)
            shift = _mm512_ternarylogic_epi32(cmp2_mask, shift_step2, shift, BIT_MERGE);

            // cmp_63_mask  = cmp(i >= 63)
            // cmp_63_value = cmp_63_mask & 29
            // shift        = shift ^ cmp_63_value
            // with ternary logic:
            // shift        = shift ^ (cmp_63_mask & 29)

            cmp_63_mask  = _mm512_cmpge_mask7bit(in, packed_byte(0x80 - 63));
#if 1
            shift = _mm512_ternarylogic_epi32(shift, cmp_63_mask, packed_byte(29), 0x78);
#else
            const __m512i cmp_63_value = _mm512_and_si512(packed_byte(29), cmp_63_mask);
            shift = _mm512_xor_si512(shift, cmp_63_value);
#endif

            // add modulo 256
            return _mm512_add_epu8(in, shift);
        }


#undef packed_dword
#undef packed_byte

    } // namespace avx512

} // namespace base64
