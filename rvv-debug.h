#pragma once

#include <cstdio>

void dump(const vuint8m1_t v) {
    uint8_t out[16];

    const size_t vlmax = __riscv_vsetvlmax_e8m1();
    __riscv_vse8_v_u8m1(out, v, vlmax);

    putchar('[');
    for (size_t i=0; i < vlmax; i++) {
        if (i == 0) {
            printf("%02x", out[i]);
        } else {
            printf("|%02x", out[i]);
        }
    }
}

void dump(const vuint16m1_t v) {
    uint16_t out[16];

    const size_t vlmax = __riscv_vsetvlmax_e16m1();
    __riscv_vse16_v_u16m1(out, v, vlmax);

    putchar('[');
    for (size_t i=0; i < vlmax; i++) {
        if (i == 0) {
            printf("%04x", out[i]);
        } else {
            printf("|%04x", out[i]);
        }
    }
    printf("]\n");
}
