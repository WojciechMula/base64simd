#include "../rvv-debug.h"

namespace base64 {
    namespace rvv {
        namespace pack {
            static uint8_t lookup_vlen16_m8[16*8];

            void initialize() {
                for (size_t i=0; i < 16*8; i++) {
                    lookup_vlen16_m8[i] = 0xff;
                }

                size_t dst = 0;
                for (size_t src=0; src < 16*8; /**/) {
                    lookup_vlen16_m8[dst + 0] = src + 2;
                    lookup_vlen16_m8[dst + 1] = src + 1;
                    lookup_vlen16_m8[dst + 2] = src + 0;
                    dst += 3;
                    src += 4;
                }
            }
        }

        vuint8m8_t pack_vlen16_m8(vuint8m8_t input) {
            const size_t vlmax = __riscv_vsetvlmax_e8m8();

            const vuint8m8_t lookup_pack = __riscv_vle8_v_u8m8(pack::lookup_vlen16_m8, vlmax);

            // input:  packed_dword([00dddddd|00cccccc|00bbbbbb|00aaaaaa])

            const vuint32m8_t input32 = __riscv_vreinterpret_v_u8m8_u32m8(input);

            const size_t vl = __riscv_vsetvlmax_e32m8();

            // db0:    packed_dword([00dddddd|00000000|00bbbbbb|00000000])
            const vuint32m8_t db0 = __riscv_vand_vx_u32m8(input32, 0x3f003f00, vl);
            // db1:    packed_dword([00000000|00dddddd|00000000|00bbbbbb])
            const vuint32m8_t db1 = __riscv_vsrl_vx_u32m8(db0, 8, vl);

            // ca0:    packed_dword([00000000|00cccccc|00000000|00aaaaaa])
            const vuint32m8_t ca0 = __riscv_vand_vx_u32m8(input32, 0x003f003f, vl);
            // ca1:    packed_dword([0000cccc|cc000000|00000aaa|aa000000])
            const vuint32m8_t ca1 = __riscv_vsll_vx_u32m8(ca0, 6, vl);

            // cdab:   packed_dword([0000cccc|ccdddddd|0000aaaa|aabbbbbb])
            const vuint32m8_t cdab = __riscv_vor_vv_u32m8(db1, ca1, vl);

            // ab0:    packed_dword([00000000|00000000|0000aaaa|aabbbbbb])
            const vuint32m8_t ab0 = __riscv_vand_vx_u32m8(cdab, 0x00000fff, vl);
            // ab1:    packed_dword([00000000|aaaaaabb|bbbb0000|00000000])
            const vuint32m8_t ab1 = __riscv_vsll_vx_u32m8(ab0, 12, vl);

            // cd0:    packed_dword([0000cccc|ccdddddd|00000000|00000000])
            const vuint32m8_t cd0 = __riscv_vand_vx_u32m8(cdab, 0x0fff0000, vl);
            // cd1:    packed_dword([00000000|00000000|0000cccc|ccdddddd])
            const vuint32m8_t cd1 = __riscv_vsrl_vx_u32m8(cd0, 16, vl);

            // abcd:   packed_dword([00000000|aaaaaabb|bbbbcccc|ccdddddd])
            const vuint32m8_t abcd = __riscv_vor_vv_u32m8(cd1, ab1, vl);

            const size_t vl1 = __riscv_vsetvlmax_e8m8();

            // pack 3 byte-groups into continous array
            return __riscv_vrgather_vv_u8m8(__riscv_vreinterpret_v_u32m8_u8m8(abcd), lookup_pack, vl1);
        }

        vuint8m8_t pack_vlen16_m8_ver2(vuint8m8_t input) {
            const size_t vl = __riscv_vsetvlmax_e8m8();

            // input:  packed_dword([00dddddd|00ccccCC|00BBbbbb|00aaaaaa])

            const vuint16m8_t input16 = __riscv_vreinterpret_v_u8m8_u16m8(input);

            // input:  packed_dword([00dddddd|00ccccCC|00BBbbbb|00aaaaaa])
            //                           CC << 14         aaaaaa << 10
            // t0 =    packed_dword([CC000000|00000000|aaaaaa00|00000000])
            const vuint32m8_t s0 = __riscv_vmv_v_x_u32m8(0x000e000a, vl);
            const vuint16m8_t t0 = __riscv_vsll_vv_u16m8(input16, __riscv_vreinterpret_v_u32m8_u16m8(s0), vl);

            // input:  packed_dword([00dddddd|00ccccCC|00BBbbbb|00aaaaaa])
            //                          cccc >> 2        BBbbbb >> 4
            // t1 =    packed_dword([0000dddd|dd00cccc|000000BB|bbbb00aa])
            //                           ^^^^^^^                ^^^^^^^^
            //                           garbage                garbage
            const vuint32m8_t s1 = __riscv_vmv_v_x_u32m8(0x00020004, vl);
            const vuint16m8_t t1 = __riscv_vsrl_vv_u16m8(input16, __riscv_vreinterpret_v_u32m8_u16m8(s1), vl);

            // t2 =    packed_dword([00000000|0000cccc|000000BB|00000000])
            const vuint32m8_t t2 = __riscv_vand_vx_u32m8(__riscv_vreinterpret_v_u16m8_u32m8(t1), 0x000f0300, vl);

            // t3 =    packed_dword([CC000000|0000cccc|aaaaaaBB|00000000])
            const vuint32m8_t t3 = __riscv_vor_vv_u32m8(__riscv_vreinterpret_v_u16m8_u32m8(t0), t2, vl);

            // input:  packed_dword([00dddddd|00ccccCC|00BBbbbb|00aaaaaa])
            // t4 =    packed_dword([ccCC00BB|bbbb00aa|aaaa0000|00000000])
            const vuint32m8_t t4 = __riscv_vsll_vx_u32m8(__riscv_vreinterpret_v_u16m8_u32m8(input16), 12, vl);

            // t5 =    packed_dword([00000000|bbbb0000|00000000|00000000])
            const vuint32m8_t t5 = __riscv_vand_vx_u32m8(t4, 0x00f00000, vl);

            // t6 =    packed_dword([CC000000|bbbbcccc|aaaaaaBB|00000000])
            const vuint32m8_t t6 = __riscv_vor_vv_u32m8(t5, t3, vl);

            // t7 =    packed_dword([00dddddd|00000000|00000000|00000000])
            const vuint32m8_t t7 = __riscv_vand_vx_u32m8(__riscv_vreinterpret_v_u16m8_u32m8(input16), 0x3f000000, vl);

            // t8 =    packed_dword([CCdddddd|bbbbcccc|aaaaaa00|00000000])
            const vuint32m8_t t8 = __riscv_vor_vv_u32m8(t6, t7, vl);

            // XXX: GCC 13.2.0 does not support intrinsic `__riscv_vreinterpret_v_u8m1_b1`
            #if 0
                const vuint8m1_t m    = __riscv_vmv_v_x_u8m1(0xee, vl); // 0b11101110
                const vbool1_t   mask = __riscv_vreinterpret_v_u8m1_b1(m);
            #else
                static const uint8_t mask_mem[16] = {
                    0xee, 0xee, 0xee, 0xee,
                    0xee, 0xee, 0xee, 0xee,
                    0xee, 0xee, 0xee, 0xee,
                    0xee, 0xee, 0xee, 0xee
                };

                const vbool1_t mask = __riscv_vlm_v_b1(mask_mem, vl);
            #endif

            return __riscv_vcompress_vm_u8m8(__riscv_vreinterpret_v_u32m8_u8m8(t8), mask, vl);
        }
    }
}
