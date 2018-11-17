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
* AVX512BW,
* AVX512VBMI,
* AVX512VL,
* BMI2, and
* ARM Neon.

Vectorization approaches were described in a series of articles:

* `Base64 encoding with SIMD instructions`__,
* `Base64 decoding with SIMD instructions`__,
* `Base64 encoding & decoding using AVX512BW instructions`__ (includes AVX512VBMI and AVX512VL),
* `AVX512F base64 coding and decoding`__.

__ http://0x80.pl/notesen/2016-01-12-sse-base64-encoding.html
__ http://0x80.pl/notesen/2016-01-17-sse-base64-decoding.html
__ http://0x80.pl/notesen/2016-04-03-avx512-base64.html
__ http://0x80.pl/articles/avx512-foundation-base64.html

`Daniel Lemire`__ and I wrote also paper `Faster Base64 Encoding
and Decoding Using AVX2 Instructions`__ which was published
by `ACM Transactiona on the Web`__.

__ http://lemire.me
__ https://arxiv.org/abs/1704.00605
__ https://tweb.acm.org/

Performance results from various machines are located
in subdirectories ``results``.


Project organization
--------------------------------------------------

There are separate subdirectories for both algorithms, however both have
the same structure. Each project contains four programs:

* ``verify`` --- does simple validation of particular parts of algorithms,
* ``check`` --- validates whole procedures,
* ``speed`` --- compares speed of different variants of procedures,
* ``benchmark`` --- similarly to ``speed`` but works on small buffers and
  calculates CPU cycle rate (available only for Intel architectures).

Building
--------------------------------------------------

Change to either directory ``encode`` or ``decode`` and then use following
``make`` commands.

.. list-table::
    :header-rows: 1

    * - command
      - tools
      - instruction sets

    * - ``make``
      - ``verify``, ``check``, ``speed``, ``benchmark``
      - scalar, SSE, BMI2

    * - ``make avx2``
      - ``verify_avx2``, ``check_avx2``, ``speed_avx2``, ``benchmark_avx2``
      - scalar, SSE, BMI2, AVX2

    * - ``make avx512``
      - ``verify_avx512``, ``check_avx512``, ``speed_avx512``, ``benchmark_avx512``
      - scalar, SSE, BMI2, AVX2, AVX512F

    * - ``make avx512bw``
      - ``verify_avx512bw``, ``check_avx512bw``, ``speed_avx512bw``, ``benchmark_avx512bw``
      - scalar, SSE, BMI2, AVX2, AVX512F, AVX512BW

    * - ``make avx512vbmi``
      - ``verify_avx512vbmi``, ``check_avx512vbmi``, ``benchmark_avx512vbmi``
      - scalar, SSE, BMI2, AVX2, AVX512F, AVX512BW, AVX512VBMI 
    
    * - ``make xop``
      - ``verify_xop``, ``check_xop``, ``speed_xop``, ``benchmark_xop``
      - scalar, SSE and AMD XOP

    * - ``make arm``
      - ``verify_arm``, ``check_arm``, ``speed_arm``
      - scalar, ARM Neon

Type ``make run`` (for SSE) or ``make run_ARCH`` to run all programs for given
instruction sets; ``ARCH`` can be "sse", "avx2", "avx512", "avx512bw",
"avx512vbmi", "avx512vl".

BMI2 presence is determined based on ``/proc/cpuinfo`` or a counterpart.
When an AVX2 or AVX512 targets are used then BMI2 is enabled by default.


AVX512
--------------------------------------------------

To compile AVX512 versions of the programs at least GCC 5.3 is required.
GCC 4.9.2 doesn't have AVX512 support.

Please download `Intel Software Development Emulator`__ in order to run AVX512
variants via ``make run_avx512``, ``run_avx512bw`` or ``run_avx512vbmi``.
The emulator path should be added to the ``PATH``.

__ https://software.intel.com/en-us/articles/intel-software-development-emulator


Known problems
--------------------------------------------------

Both encoding and decoding don't match the base64 specification,
there is no processing of data tail, i.e. encoder never produces
'=' chars at the end, and decoder doesn't handle them at all.

All these shortcoming are not present in a brilliant library
by Alfred Klomp: https://github.com/aklomp/base64.


See also
--------------------------------------------------

* Daniel's benchmarks and comparison with state of the art solutions
  https://github.com/lemire/fastbase64
