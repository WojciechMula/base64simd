
namespace base64 {

    namespace scalar {

        /*
            Note: function doesn't encode few tail bytes as the
                  single step is 3 bytes. The method is not intended
                  to be production-ready. Sorry for that.
        */
        void encode32(const uint8_t* input, size_t bytes, uint8_t* output) {

            static const char* lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 3) {

                // [????????|ccdddddd|bbbbcccc|aaaaaabb]
                //           ^^       ^^^^^^^^       ^^
                //           lo        lo  hi        hi

                const uint32_t dword   = *reinterpret_cast<const uint32_t*>(input + i);

                // [aaaaaabb|bbbbcccc|ccdddddd|????????]
                const uint32_t swapped = __builtin_bswap32(dword);

                const auto a = (swapped >> 26) & 0x3f;
                const auto b = (swapped >> 20) & 0x3f;
                const auto c = (swapped >> 14) & 0x3f;
                const auto d = (swapped >>  8) & 0x3f;

                *out++ = lookup[a];
                *out++ = lookup[b];
                *out++ = lookup[c];
                *out++ = lookup[d];
            }
        }

        void encode64(const uint8_t* input, size_t bytes, uint8_t* output) {

            static const char* lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            uint8_t* out = output;

            for (size_t i = 0; i < bytes; i += 6) {

                const uint64_t dword = *reinterpret_cast<const uint64_t*>(input + i);
                const uint64_t swapped = __builtin_bswap64(dword);

                const auto a = (swapped >> (26 + 4*8)) & 0x3f;
                const auto b = (swapped >> (20 + 4*8)) & 0x3f;
                const auto c = (swapped >> (14 + 4*8)) & 0x3f;
                const auto d = (swapped >> ( 8 + 4*8)) & 0x3f;
                const auto e = (swapped >> (26 + 1*8)) & 0x3f;
                const auto f = (swapped >> (20 + 1*8)) & 0x3f;
                const auto g = (swapped >> (14 + 1*8)) & 0x3f;
                const auto h = (swapped >> ( 8 + 1*8)) & 0x3f;

                *out++ = lookup[a];
                *out++ = lookup[b];
                *out++ = lookup[c];
                *out++ = lookup[d];
                *out++ = lookup[e];
                *out++ = lookup[f];
                *out++ = lookup[g];
                *out++ = lookup[h];
            }
        }

    } // namespace scalar

} // namespace base64
