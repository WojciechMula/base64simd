#include "../rvv-debug.h"

namespace base64 {
    namespace rvv {

        // Direct translation of base64::sse::lookup_pshufb_improved (see: lookup.see.cpp)
        vuint8m1_t lookup_pshufb_improved(const vuint8m1_t input, size_t vl) {
            // reduce  0..51 -> 0
            //        52..61 -> 1 .. 10
            //            62 -> 11
            //            63 -> 12
            const vuint8m1_t t0 = __riscv_vssubu(input, 51, vl);

            // distinguish between ranges 0..25 and 26..51:
            //         0 .. 25 -> becomes 13
            //        26 .. 51 -> remains 0
            const vbool8_t   lt = __riscv_vmsltu_vx_u8m1_b8(input, 26, vl);

            const vuint8m1_t t1 = __riscv_vadd_vx_u8m1_mu(lt, t0, t0, 13, vl);

            const uint8_t shift_lut_arr[16] = {
                uint8_t('a' - 26),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('0' - 52),
                uint8_t('+' - 62),
                uint8_t('/' - 63),
                uint8_t('A'),
                0,
                0
            };
            const size_t     vlmax     = __riscv_vsetvlmax_e8m1();
            const vuint8m1_t shift_lut = __riscv_vle8_v_u8m1(shift_lut_arr, vlmax);

            // read shift
            const vuint8m1_t shift = __riscv_vrgather_vv_u8m1(shift_lut, t1, vl);

            return __riscv_vadd(input, shift, vl);
        }

        const uint8_t lookup_vlen16_m8[16*8] = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
            'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        vuint8m8_t lookup_wide_gather(vuint8m8_t indices, size_t /*unused*/) {
            const size_t vl = __riscv_vsetvlmax_e8m8();

            const vuint8m8_t lookup = __riscv_vle8_v_u8m8(lookup_vlen16_m8, vl);

            return __riscv_vrgather_vv_u8m8(lookup, indices, vl);
        }

        vuint8m4_t lookup_option(vuint8m4_t indices, size_t vl)
        {
            const int8_t offsets[68] = {71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 65, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            vint8m1_t offset_vec = __riscv_vle8_v_i8m1(offsets, vl);

            vl = __riscv_vsetvlmax_e8m4();
            // reduce values 0-64 to 0-13
            vuint8m4_t result = __riscv_vssubu_vx_u8m4(indices, 51, vl);
            vbool2_t vec_lt_26 = __riscv_vmsltu_vx_u8m4_b2(indices, 26, vl);
            const vuint8m4_t vec_lookup = __riscv_vadd_vx_u8m4_mu(vec_lt_26, result, result, 13, vl);

            // shuffle registers one by one
            vint8m1_t offset_vec_0 = __riscv_vrgather_vv_i8m1(offset_vec, __riscv_vget_v_u8m4_u8m1(vec_lookup, 0), vl);
            vint8m1_t offset_vec_1 = __riscv_vrgather_vv_i8m1(offset_vec, __riscv_vget_v_u8m4_u8m1(vec_lookup, 1), vl);
            vint8m1_t offset_vec_2 = __riscv_vrgather_vv_i8m1(offset_vec, __riscv_vget_v_u8m4_u8m1(vec_lookup, 2), vl);
            vint8m1_t offset_vec_3 = __riscv_vrgather_vv_i8m1(offset_vec, __riscv_vget_v_u8m4_u8m1(vec_lookup, 3), vl);

            vint8m4_t offset_vec_bundle = __riscv_vcreate_v_i8m1_i8m4(offset_vec_0, offset_vec_1, offset_vec_2, offset_vec_3);

            vint8m4_t ascii_vec = __riscv_vadd_vv_i8m4(__riscv_vreinterpret_v_u8m4_i8m4(indices), offset_vec_bundle, vl);

            return __riscv_vreinterpret_v_i8m4_u8m4(ascii_vec);
        }

    } // namespace rvv

} // namespace base64
