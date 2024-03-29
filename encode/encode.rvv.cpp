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

            // scalar fallback
            base64::scalar::encode32(input, bytes, out);
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

            // scalar fallback
            base64::scalar::encode32(input, bytes, out);
        }

        void encode_loadseg(const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const size_t VLEN = 16;
            const size_t LMUL = 2;

            asm volatile (
                // preload 64-byte lookup for conversion from 6-bit values into Base64 ASCII values
                "vsetvli        t0, zero, e8, m8\n"
                "vl8re8.v       v24, (%[lookup])\n"
            "1:\n"
                // while (bytes >= input_inc)
                "li             t0, %[input_inc]\n"
                "bltu           %[bytes], t0, 2f\n"

                "vsetvli        t0, zero, e8, m2\n"
                "vlseg3e8.v     v0, (%[input])\n"
                // [CCdddddd|bbbbcccc|aaaaaaBB] x VLEN x LMUL
                //   byte 2   byte 1   byte 0
                //
                // v{0..1} = byte 0
                // v{2..3} = byte 1
                // v{4..5} = byte 2

                // v{6..7} = [00dddddd] - byte 3 of output
                "li             t0, 0x3f\n"
                "vand.vx        v6, v4, t0\n"

                // v{12..13} = [00aaaaaa] - byte 0 of output
                "vsrl.vi        v12, v0, 2\n"

                // v{10..11} = [000000CC]
                "vsrl.vi        v4, v4, 6\n"

                // v{8..9}   = [00cccc00]
                "vand.vi        v8, v2, 0xf\n"
                "vsll.vi        v8, v8, 2\n"

                // v{2..3}   = [00ccccCC] - byte 2 of output
                "vor.vv         v4, v8, v4\n"

                // v{8..9}   = [0000bbbb]
                "vsrl.vi        v8, v2, 4\n"
                // v{0..1}   = [000000BB]
                "vand.vi        v0, v0, 0x03\n"
                // v{0..1}   = [00BB0000]
                "vsll.vi        v0, v0, 4\n"
                // v{0..3}   = [00BBbbbb] - byte 1 of output
                "vor.vv         v2, v0, v8\n"

                // prepare layout
                "vmv.v.v        v0, v12\n"  // byte 0
                                            // byte 1 - already in v2
                                            // byte 2 - already in v4
                                            // byte 3 - already in v6

                // translate 6-bit values => ASCII
                "vsetvli        t0, zero, e8, m8\n"
                "vrgather.vv    v8, v24, v0\n"

                "vsetvli        t0, zero, e8, m2\n"
                "vsseg4e8.v     v8, (%[output])\n"

                "li             t0, %[output_inc]\n"
                "add            %[output], %[output], t0\n"

                "li             t0, %[input_inc]\n"
                "add            %[input], %[input], t0\n"
                "sub            %[bytes], %[bytes], t0\n"

                "j              1b\n"
            "2:\n"
                /* outputs */
                :
                /* inputs */
                : [input]  "r" (input)
                , [output] "r" (out)
                , [bytes]  "r" (bytes)
                , [lookup] "r" (base64::rvv::lookup_vlen16_m8)
                , [input_inc] "i" (3 * VLEN * LMUL)
                , [output_inc] "i" (4 * VLEN * LMUL)
                /* clobbers */
                : "t0", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13"
            );

            // scalar fallback
            base64::scalar::encode32(input, bytes, out);
        }

        template <typename LOOKUP_FN>
        void encode_strided_load_m8(LOOKUP_FN lookup, const uint8_t* input, size_t bytes, uint8_t* output) {

            uint8_t* out = output;

            const size_t VLEN = 16;
            const size_t LMUL = 8;
            const size_t vl = __riscv_vsetvlmax_e8m8();

            const size_t input_inc  = 3 * (VLEN * LMUL);
            const size_t output_inc = 4 * (VLEN * LMUL);
            while (bytes >= input_inc) {
                // input:  [CCdddddd|bbbbcccc|aaaaaaBB] x VLEN x LMUL
                //           byte 2   byte 1   byte 0
                //
                // output: [00dddddd|00ccccCC|00BBbbbb|00aaaaa]
                //           byte 3   byte 2   byte 1   byte 0

                // load byte 0
                const vuint8m8_t byte0 = __riscv_vlse8_v_u8m8(input + 0, 3, vl);

                // load byte 1
                const vuint8m8_t byte1 = __riscv_vlse8_v_u8m8(input + 1, 3, vl);

                // extract and store field 'a'
                {
                    // t0 = [00aaaaaaa]
                    const vuint8m8_t t0 = __riscv_vsrl_vx_u8m8(byte0, 2, vl);
                    __riscv_vsse8_v_u8m8(out + 0, 4, lookup(t0, vl), vl);
                }

                // extract and store field 'b'
                {
                    // t0 = [0000bbbbb]
                    const vuint8m8_t t0 = __riscv_vsrl_vx_u8m8(byte1, 4, vl);

                    // t1 = [0000000BB]
                    const vuint8m8_t t1 = __riscv_vand_vx_u8m8(byte0, 0x03, vl);

                    // t2 = [00BB00000]
                    const vuint8m8_t t2 = __riscv_vsll_vx_u8m8(t1, 4, vl);

                    // t3 = [00BBbbbbb]
                    const vuint8m8_t t3 = __riscv_vor_vv_u8m8(t0, t2, vl);

                    __riscv_vsse8_v_u8m8(out + 1, 4, lookup(t3, vl), vl);
                }

                // load byte 2
                const vuint8m8_t byte2 = __riscv_vlse8_v_u8m8(input + 2, 3, vl);

                // extract and store field 'c'
                {
                    // t0 = [0000cccc]
                    const vuint8m8_t t0 = __riscv_vand_vx_u8m8(byte1, 0x0f, vl);

                    // t1 = [00cccc00]
                    const vuint8m8_t t1 = __riscv_vsll_vx_u8m8(t0, 2, vl);

                    // t2 = [CC000000]
                    const vuint8m8_t t2 = __riscv_vand_vx_u8m8(byte2, 0xc0, vl);

                    // t3 = [00CC0000]
                    const vuint8m8_t t3 = __riscv_vsrl_vx_u8m8(t2, 6, vl);

                    // t3 = [00CCccccc]
                    const vuint8m8_t t4 = __riscv_vor_vv_u8m8(t1, t3, vl);

                    __riscv_vsse8_v_u8m8(out + 2, 4, lookup(t4, vl), vl);
                }

                // extract and store field 'd'
                {
                    // t0 = [00dddddd]
                    const vuint8m8_t t0 = __riscv_vand_vx_u8m8(byte2, 0x3f, vl);

                    __riscv_vsse8_v_u8m8(out + 3, 4, lookup(t0, vl), vl);
                }

                input += input_inc;
                bytes -= input_inc;
                out   += output_inc;
            }

            // scalar fallback
            base64::scalar::encode32(input, bytes, out);
        }
    } // namespace rvv
} // namespace base64

