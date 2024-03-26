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

        template <typename FN_LOOKUP, typename FN_PACK>
        uint8_t* decode_vlen16_m8_omit_ws(FN_LOOKUP lookup, FN_PACK pack, const uint8_t* input, size_t size, uint8_t* output) {
            const size_t VLEN = 16;
            const size_t VEC_LEN = VLEN*8;

            uint8_t stash[VEC_LEN * 2];
            size_t stash_size = 0;

            const uint8_t* start = input;
            uint8_t* out = output;

            while (size >= VEC_LEN) {
                const size_t vl = __riscv_vsetvl_e8m8(size);
                const vuint8m8_t in = __riscv_vle8_v_u8m8(input, vl);
                int err_pos = -1;

                const vuint8m8_t translated = lookup(in, err_pos);

                if (err_pos >= 0) {
                    if (size_t(err_pos) < vl) {
                        const size_t offset = input - start;
                        throw invalid_input(offset + err_pos, input[offset + err_pos]);
                    }
                }

                size -= vl;
                input += vl;

                const vbool1_t non_ws = __riscv_vmsne_vx_u8m8_b1(translated, 0x7f, vl);
                const size_t non_ws_count = __riscv_vcpop_m_b1(non_ws, vl);
                if (stash_size == 0 && non_ws_count == vl) {
                    // fast path
                    const vuint8m8_t result = pack(translated);
                    const size_t vecsize = (VEC_LEN/4) * 3;
                    const size_t vl = __riscv_vsetvl_e8m8(vecsize);

                    __riscv_vse8_v_u8m8(out, result, vl);
                    out += vl;
                    continue;
                }

                const vuint8m8_t compressed = __riscv_vcompress_vm_u8m8(translated, non_ws, vl);
                {
                    size_t vl = __riscv_vsetvl_e8m8(non_ws_count);
                    __riscv_vse8_v_u8m8(stash + stash_size, compressed, vl);
                    stash_size += non_ws_count;
                }

                if (stash_size >= VEC_LEN) {
                    size_t vlmax = __riscv_vsetvlmax_e8m8();
                    const vuint8m8_t translated = __riscv_vle8_v_u8m8(stash, vlmax);

                    const vuint8m8_t result = pack(translated);
                    const size_t vecsize = (VEC_LEN/4) * 3;
                    const size_t vl = __riscv_vsetvl_e8m8(vecsize);

                    __riscv_vse8_v_u8m8(out, result, vl);
                    out += vl;

                    {
                        stash_size -= VEC_LEN;
                        size_t vl = __riscv_vsetvl_e8m8(stash_size);
                        const vuint8m8_t tmp = __riscv_vle8_v_u8m8(stash + VEC_LEN, vl);
                        __riscv_vse8_v_u8m8(stash, tmp, vl);
                    }
                }
            }

            uint8_t* bytes = stash;
            while (stash_size > 4) {
                *out++ = (bytes[1] >> 4) | (bytes[0] << 2);
                *out++ = (bytes[2] >> 2) | (bytes[1] << 4);
                *out++ = bytes[3] | (bytes[2] << 6);
                stash_size -= 4;
                bytes += 4;
            }

            switch (stash_size) {
                case 1: {
                    *out++ = (bytes[0] << 2);
                    break;
                }
                case 2: {
                    *out++ = (bytes[1] >> 4) | (bytes[0] << 2);
                    *out++ = (bytes[1] << 4);
                    break;
                }
                case 3: {
                    *out++ = (bytes[1] >> 4) | (bytes[0] << 2);
                    *out++ = (bytes[2] >> 2) | (bytes[1] << 4);
                    *out++ = (bytes[2] << 6);
                    break;
                }
                case 4: {
                    *out++ = (bytes[1] >> 4) | (bytes[0] << 2);
                    *out++ = (bytes[2] >> 2) | (bytes[1] << 4);
                    *out++ = bytes[3] | (bytes[2] << 6);
                    break;
                }
            }

            return out;
        }
    }
}
