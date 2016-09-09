
namespace base64 {

    namespace swar {

        template <int shift>
        uint64_t unpack(uint64_t x, uint64_t lomask, uint64_t himask) {
            
            return ((x << shift) & himask) | (x & lomask);
        }

        void encode(uint8_t* input, size_t bytes, uint8_t* output) {

            uint64_t* out = reinterpret_cast<uint64_t*>(output);

            for (size_t i = 0; i < bytes; i += 6) {


#if defined(HAVE_BMI2_INSTRUCTIONS)
                const uint64_t indices = pdep(*reinterpret_cast<uint64_t*>(input + i), 0x3f3f3f3f3f3f3f3flu);
#else
                const uint64_t dword   = *reinterpret_cast<uint64_t*>(input + i);

                const uint64_t tmp1    = unpack<8>(dword, 0x0000000000fffffflu, 0x00ffffff00000000lu);
                const uint64_t tmp2    = unpack<4>(tmp1,  0x00000fff00000ffflu, 0x0fff00000fff0000lu);
                const uint64_t indices = unpack<2>(tmp2,  0x003f003f003f003flu, 0x3f003f003f003f00lu);
#endif
                *out++ = lookup_incremental_logic(indices);
            }

        }

    } // namespace scalar

} // namespace base64
