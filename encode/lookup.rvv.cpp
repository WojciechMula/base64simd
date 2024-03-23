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

    } // namespace rvv

} // namespace base64
