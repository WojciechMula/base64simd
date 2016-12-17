
namespace base64 {

    namespace swar {


        void encode(const uint8_t* input, size_t bytes, uint8_t* output) {

            uint64_t* out = reinterpret_cast<uint64_t*>(output);

            for (size_t i = 0; i < bytes; i += 6) {

#if defined(HAVE_BMI2_INSTRUCTIONS)
                // in      = [?|?|f|e|d|c|b|a]
                const uint64_t in      = *reinterpret_cast<const uint64_t*>(input + i);
                // swapped = [a|b|c|d|e|f|?|?]
                const uint64_t swapped = __builtin_bswap64(in);
                // input   = [.|.|d|e|f|a|b|c]
                const uint64_t input   = ((swapped << 1*8) & 0x0000ffffff000000llu)
                                       | ((swapped >> 5*8) & 0x0000000000ffffffllu);
                const uint64_t t0      = _pdep_u64(input, 0x3f3f3f3f3f3f3f3flu);
                const uint64_t lo      = __builtin_bswap32(uint32_t(t0));
                const uint64_t hi      = __builtin_bswap32(t0 >> 32);

                const uint64_t indices = lo | (hi << 32);
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
