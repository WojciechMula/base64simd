================================================================================
                        base64 using SIMD instructions
================================================================================

Overview
--------------------------------------------------

Repository contains code for encoding and decoding base64 using SIMD instructions.
Depending on CPU's architecture, vectorized encoding is faster than scalar
versions by factor from **2 to 4**; decoding is faster **2 .. 2.7** times.

There are several versions of procedures utilizing following instructions sets:

* SSE,
* AVX2,
* AVX512F,
* AVX512BW, and
* BMI2

Vectorization approaches were described in a series of articles:

* `Base64 encoding with SIMD instructions`__,
* `Base64 decoding with SIMD instructions`__,
* `Base64 encoding & decoding using AVX512 instructions`__ (only AVX512BW)

__ ../notesen/2016-01-12-sse-base64-encoding.html
__ ../notesen/2016-01-17-sse-base64-decoding.html
__ ../notesen/2016-04-03-avx512-base64.html

Performance results from various machines are located
in subdirectories ``results``.


Project organization
--------------------------------------------------

There are separate subdirectories for both algorithms, however both have
the same structure. Each projects has builds three programs:

* ``verify`` --- makes simple validation of particular parts of algorithms,
* ``check`` --- validates procedures,
* ``speed`` --- compares speed of different variants of procedures.


Building
--------------------------------------------------

Type ``make`` to build ``verify``, ``check`` and ``speed`` program.  The
programs support only scalar, SSE and BMI versions of procedures.

Type ``make avx2`` to build ``verify_avx2``, ``check_avx2`` and ``speed_avx2``.
The programs additionally support AVX2 variants

Type ``make avx512`` to build ``verify_avx512``, ``check_avx512`` and
``speed_avx512``.  The programs support also AVX512F variants.

Type ``make avx512BW`` to build ``verify_avx512BW``, ``check_avx512BW`` and
``speed_avx512BW``.  The programs support also AVX512BW variants.

Type ``make run``, ``make run_avx2``, ``make run_avx512`` or ``make run_avx512bw``
to run all programs.


AVX512
--------------------------------------------------

To compile AVX512 versions of the programs at least GCC 5.3 is required.
GCC 4.9.2 haven't got AVX512 support.

Please download `Intel Software Development Emulator`__ in order to run AVX512
variants via ``make run_avx512`` or ``run_avx512bw``.  The emulator path should
be added to the ``PATH``.

__ https://software.intel.com/en-us/articles/intel-software-development-emulator

