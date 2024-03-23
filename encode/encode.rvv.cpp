// bytes from groups A, B and C are needed in separate 32-bit lanes
// in = [DDDD|CCCC|BBBB|AAAA]
//
//      an input triplet has layout
//      [????????|ccdddddd|bbbbcccc|aaaaaabb]
//        byte 3   byte 2   byte 1   byte 0    -- byte 3 comes from the next triplet
//
//      shuffling changes the order of bytes: 1, 0, 2, 1
//      [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
//           ^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^
//                  processed bits
//
#define EXPAND(idx) (idx) + 1, (idx), (idx) + 2, (idx) + 1

const uint8_t expand_vlen16_m8[16*8] = {
    EXPAND(3*0),
    EXPAND(3*1),
    EXPAND(3*2),
    EXPAND(3*3),

    EXPAND(3*4),
    EXPAND(3*5),
    EXPAND(3*6),
    EXPAND(3*7),

    EXPAND(3*8),
    EXPAND(3*9),
    EXPAND(3*10),
    EXPAND(3*11),

    EXPAND(3*12),
    EXPAND(3*13),
    EXPAND(3*14),
    EXPAND(3*15),

    EXPAND(3*16),
    EXPAND(3*17),
    EXPAND(3*18),
    EXPAND(3*19),

    EXPAND(3*20),
    EXPAND(3*21),
    EXPAND(3*22),
    EXPAND(3*23),

    EXPAND(3*24),
    EXPAND(3*25),
    EXPAND(3*26),
    EXPAND(3*27),

    EXPAND(3*28),
    EXPAND(3*29),
    EXPAND(3*30),
    EXPAND(3*31),
};

namespace base64 {
    namespace rvv {
        template <typename LOOKUP_FN>
        void encode(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const size_t     vl   = __riscv_vsetvlmax_e8m1();
            const vuint8m1_t shuf = __riscv_vle8_v_u8m1(expand_vlen16_m8, vl);

            while (bytes > 0) {
                const vuint8m1_t in = __riscv_vle8_v_u8m1(input, vl);
                input += 3*4;
                bytes -= 3*4;

                // in32  = [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
                const vuint8m1_t  tmp  = __riscv_vrgather_vv_u8m1(in, shuf, vl);
                const vuint32m1_t in32 = __riscv_vreinterpret_v_u8m1_u32m1(tmp);

                // unpacking

                // ca_t0 = [0000cccc|cc000000|aaaaaa00|00000000]
                const vuint32m1_t ca32_t0    = __riscv_vand_vx_u32m1(in32, 0x0fc0fc00, vl);
                const vuint16m1_t ca_t0      = __riscv_vreinterpret_v_u32m1_u16m1(ca32_t0);
                const vuint32m1_t ca32_shift = __riscv_vmv_v_x_u32m1(0x0006000a, vl);
                const vuint16m1_t ca_shift   = __riscv_vreinterpret_v_u32m1_u16m1(ca32_shift);

                // ca_t1 = [00000000|00cccccc|00000000|00aaaaaa]
                //          field c >> 6      field a >> 10
                const vuint16m1_t ca_t1    = __riscv_vsrl_vv_u16m1(ca_t0, ca_shift, vl);

                // db_t0  = [00000000|00dddddd|000000bb|bbbb0000]
                const vuint32m1_t db32_t0    = __riscv_vand_vx_u32m1(in32, 0x003f03f0, vl);
                const vuint16m1_t db_t0      = __riscv_vreinterpret_v_u32m1_u16m1(db32_t0);
                const vuint32m1_t db32_shift = __riscv_vmv_v_x_u32m1(0x00080004, vl);
                const vuint16m1_t db_shift   = __riscv_vreinterpret_v_u32m1_u16m1(db32_shift);

                // db_t1  = [00dddddd|00000000|00bbbbbb|00000000](
                //          field d << 8       field b << 4
                const vuint16m1_t db_t1    = __riscv_vsll_vv_u16m1(db_t0, db_shift, vl);

                // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
                const vuint16m1_t indices16 = __riscv_vor_vv_u16m1(ca_t1, db_t1, vl);
                const vuint8m1_t  indices   = __riscv_vreinterpret_v_u16m1_u8m1(indices16);

                const vuint8m1_t  result    = lookup(indices, vl);

                __riscv_vse8_v_u8m1(out, result, vl);
                out += vl;
            }
        }

        template <typename LOOKUP_FN>
        void encode_m8(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const size_t     vl   = __riscv_vsetvlmax_e8m8();
            const vuint8m8_t shuf = __riscv_vle8_v_u8m8(expand_vlen16_m8, vl);

            const size_t step = 3*(4*8);
            while (bytes >= step) {
                const vuint8m8_t in = __riscv_vle8_v_u8m8(input, vl);
                input += step;
                bytes -= step;

                // in32  = [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]
                const vuint8m8_t  tmp  = __riscv_vrgather_vv_u8m8(in, shuf, vl);
                const vuint32m8_t in32 = __riscv_vreinterpret_v_u8m8_u32m8(tmp);

                // unpacking

                // ca_t0 = [0000cccc|cc000000|aaaaaa00|00000000]
                const vuint32m8_t ca32_t0    = __riscv_vand_vx_u32m8(in32, 0x0fc0fc00, vl);
                const vuint16m8_t ca_t0      = __riscv_vreinterpret_v_u32m8_u16m8(ca32_t0);
                const vuint32m8_t ca32_shift = __riscv_vmv_v_x_u32m8(0x0006000a, vl);
                const vuint16m8_t ca_shift   = __riscv_vreinterpret_v_u32m8_u16m8(ca32_shift);

                // ca_t1 = [00000000|00cccccc|00000000|00aaaaaa]
                //          field c >> 6      field a >> 10
                const vuint16m8_t ca_t1    = __riscv_vsrl_vv_u16m8(ca_t0, ca_shift, vl);

                // db_t0  = [00000000|00dddddd|000000bb|bbbb0000]
                const vuint32m8_t db32_t0    = __riscv_vand_vx_u32m8(in32, 0x003f03f0, vl);
                const vuint16m8_t db_t0      = __riscv_vreinterpret_v_u32m8_u16m8(db32_t0);
                const vuint32m8_t db32_shift = __riscv_vmv_v_x_u32m8(0x00080004, vl);
                const vuint16m8_t db_shift   = __riscv_vreinterpret_v_u32m8_u16m8(db32_shift);

                // db_t1  = [00dddddd|00000000|00bbbbbb|00000000](
                //          field d << 8       field b << 4
                const vuint16m8_t db_t1    = __riscv_vsll_vv_u16m8(db_t0, db_shift, vl);

                // res   = [00dddddd|00cccccc|00bbbbbb|00aaaaaa] = t1 | t3
                const vuint16m8_t indices16 = __riscv_vor_vv_u16m8(ca_t1, db_t1, vl);
                const vuint8m8_t  indices   = __riscv_vreinterpret_v_u16m8_u8m8(indices16);

                const vuint8m8_t  result    = lookup(indices, vl);

                __riscv_vse8_v_u8m8(out, result, vl);
                out += vl;
            }
        }
    } // namespace rvv
} // namespace base64

