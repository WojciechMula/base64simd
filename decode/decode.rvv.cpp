namespace base64 {
    namespace rvv {
        template <typename FN_LOOKUP, typename FN_PACK>
        void decode_vlen16_m8(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {

            assert(size % (8*16) == 0);

            uint8_t* out = output;

            for (size_t i=0; i < size; i += 8*16) {
                const size_t vlmax = __riscv_vsetvlmax_e8m8();
                const vuint8m8_t in = __riscv_vle8_v_u8m8(input, vlmax);
                int err_pos = -1;

                const vuint8m8_t translated = lookup(in, err_pos);

                if (err_pos >= 0) {
                    throw invalid_input(i + err_pos, input[i + err_pos]);
                }

                const vuint8m8_t result = pack(translated);

                const size_t vecsize = (8*16/4) * 3;
                const size_t vl = __riscv_vsetvl_e8m8(vecsize);

                __riscv_vse8_v_u8m8(out, result, vl);
                out += vl;
            }
        }

    }
}
