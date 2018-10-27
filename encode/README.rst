Base64 encoding with SIMD instructions
--------------------------------------------------------------------------------

Please read README.rst from the root directory.


Programs details
--------------------------------------------------

* ``verify`` --- verifies branchless procedures calculating the lookup table
* ``check`` --- checks if base64 encoders work correctly
* ``speed`` --- allows to measure speed of all or selected base64 encoder
  it encodes 192 MiB of artificial data 10 times, and then print the smallest
  measurement.
* ``benchmark`` --- measure CPU cycles per input byte of all or selected encoder
