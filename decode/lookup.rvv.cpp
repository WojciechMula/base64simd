namespace base64 {
    namespace rvv {

        namespace lookup {
            static uint8_t lookup_vlen16_m8[16*8];
            static uint8_t lookup_vlen16_m8_omit_ws[16*8];

            void initialize() {
                // bytes having MSB set will be rejected
                for (int i=0; i < 16*8; i++) {
                    lookup_vlen16_m8[i] = 0x80;
                    lookup_vlen16_m8_omit_ws[i] = 0x80;
                }

                for (int i=0; i < 64; i++) {
                    const uint8_t val = static_cast<uint8_t>(base64::lookup[i]);
                    lookup_vlen16_m8[val] = i;
                    lookup_vlen16_m8_omit_ws[val] = i;
                }

                lookup_vlen16_m8_omit_ws[' '] = 0x7f;
                lookup_vlen16_m8_omit_ws['\n'] = 0x7f;
                lookup_vlen16_m8_omit_ws['\r'] = 0x7f;
                lookup_vlen16_m8_omit_ws['\t'] = 0x7f;
                lookup_vlen16_m8_omit_ws['\v'] = 0x7f;
            }
        }

        vuint8m8_t lookup_vlen16_m8(const vuint8m8_t input, int& err_pos) {
            const size_t vl             = __riscv_vsetvlmax_e8m8();
            // input[i] & 0x7f => base64 characters or 0x80 on error
            const vuint8m8_t lookup     = __riscv_vle8_v_u8m8(lookup::lookup_vlen16_m8, vl);
            const vuint8m8_t translated = __riscv_vrgather_vv_u8m8(lookup, input, vl);

            const vuint8m8_t t0 = __riscv_vor_vv_u8m8(translated, input, vl);
            const vint8m8_t  t1 = __riscv_vreinterpret_v_u8m8_i8m8(t0);

            const vbool1_t   err = __riscv_vmslt_vx_i8m8_b1(t1, 0, vl);
            err_pos = __riscv_vfirst_m_b1(err, vl);

            return translated;
        }

        vuint8m8_t lookup_vlen16_m8_omit_ws(const vuint8m8_t input, int& err_pos) {
            const size_t vl             = __riscv_vsetvlmax_e8m8();
            const vuint8m8_t lookup     = __riscv_vle8_v_u8m8(lookup::lookup_vlen16_m8_omit_ws, vl);
            const vuint8m8_t translated = __riscv_vrgather_vv_u8m8(lookup, input, vl);

            const vuint8m8_t t0 = __riscv_vor_vv_u8m8(translated, input, vl);
            const vint8m8_t  t1 = __riscv_vreinterpret_v_u8m8_i8m8(t0);

            const vbool1_t   err = __riscv_vmslt_vx_i8m8_b1(t1, 0, vl);
            err_pos = __riscv_vfirst_m_b1(err, vl);

            return translated;
        }

    } // namespace sse
} // namespace base64
