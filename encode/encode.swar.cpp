
namespace base64 {

    namespace swar {


        void encode(const uint8_t* input, size_t bytes, uint8_t* output) {

            uint64_t* out = reinterpret_cast<uint64_t*>(output);

            for (size_t i = 0; i < bytes; i += 6) {

#if defined(HAVE_BMI2_INSTRUCTIONS)
                const uint64_t indices = _pdep_u64(*reinterpret_cast<const uint64_t*>(input + i), 0x3f3f3f3f3f3f3f3flu);
#else
                const uint64_t in      = *reinterpret_cast<const uint64_t*>(input + i);
                const uint64_t swapped = __builtin_bswap64(in);

                const uint64_t input   = ((swapped << 3*8) & 0xffffff0000000000llu)
                                       | ((swapped >> 4*8) & 0x00000000ffffff00llu);

                const uint64_t a       = (input >> 26) & 0x0000003f0000003fllu;
                const uint64_t b       = (input >> 12) & 0x00003f0000003f00llu;
                const uint64_t c       = (input <<  2) & 0x003f0000003f0000llu;
                const uint64_t d       = (input << 16) & 0x3f0000003f000000llu;

                const uint64_t indices = a | b | c | d;
#endif
                *out++ = lookup_incremental_logic(indices);
            }

        }

    } // namespace scalar

} // namespace base64
