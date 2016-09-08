Base64 encoding with SIMD instructions
================================================================================

Please read README.rst from the root directory.


Programs details
--------------------------------------------------

* ``verify`` --- verifies all decoding procedures
* ``check`` --- checks if base64 decoders work correctly
* ``speed`` --- allows to measure speed of all or selected base64 decoder;
  it decodes 64 MiB of artificial data 10 times, and then print the smallest
  measurement.

