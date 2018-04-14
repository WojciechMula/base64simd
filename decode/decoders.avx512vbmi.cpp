#include "lookup.avx512vbmi.cpp"
#include "decode.avx512vbmi.cpp"

namespace base64 {

    namespace avx512vbmi {

        void prepare_lookups() {

            initalize_lookup();
            initalize_decode();
        }

    } // namespace avx512bw

} // namespace base64
