#pragma once

// helper macros and functions
#define packed_byte(x) vdupq_n_u8(x)

#define vtbl2q_u8(lookup, qreg) vcombine_u8( \
                                    vtbl2_u8(lookup, vget_low_u8(qreg)), \
                                    vtbl2_u8(lookup, vget_high_u8(qreg))); 
