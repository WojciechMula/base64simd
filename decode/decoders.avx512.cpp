#include "../avx512_swar.cpp"
#include "pack.avx512.cpp"
#include "lookup.avx512.cpp"
#include "decode.avx512.cpp"

namespace base64 {

    namespace avx512 {

        void prepare_lookups() {

            initalize_lookup();
        }

    } // namespace avx512

} // namespace base64
