.SUFFIXES:
.PHONY: all avx2 avx512 avx512bw

SDE=sde -cnl

FLAGS_BASE=-Wall -pedantic -O3 -std=c++11
HAVE_BMI2=$(shell python script/cpuflags.py bmi2)
ifeq ($(HAVE_BMI2),present)
    FLAGS_BASE+=-DHAVE_BMI2_INSTRUCTIONS
endif
FLAGS=$(FLAGS_BASE) -msse4
FLAGS_AVX2=$(FLAGS_BASE) -mavx2 -mbmi2 -DHAVE_AVX2_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512=$(FLAGS_BASE) -mavx512f -mbmi2 -DHAVE_AVX512_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512BW=$(FLAGS_BASE) -mavx512vbmi -mbmi2 -DHAVE_AVX512_INSTRUCTIONS -DHAVE_AVX512BW_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_XOP=$(FLAGS_BASE) -mxop -DHAVE_XOP_INSTRUCTIONS

EXT_DEPS=../fnv32.cpp ../gettime.cpp

ALL=verify speed check
ALL_AVX2=verify_avx2 speed_avx2 check_avx2
ALL_AVX512=verify_avx512 check_avx512 speed_avx512
ALL_AVX512BW=check_avx512bw
ALL_XOP=verify_xop speed_xop check_xop
ALL_TARGETS=$(ALL) $(ALL_AVX2) $(ALL_AVX512) $(ALL_AVX512BW) $(ALL_XOP)

all: $(ALL)

sse: $(ALL)

avx2: $(ALL_AVX2)

avx512: $(ALL_AVX512)

avx512bw: $(ALL_AVX512BW)

xop: $(ALL_XOP)

run: $(ALL)
	./verify
	./check
	./speed

run_avx2: $(ALL_AVX2)
	./verify_avx2
	./check_avx2
	./speed_avx2

run_avx512: $(ALL_AVX512)
	$(SDE) -- ./verify_avx512
	$(SDE) -- ./check_avx512

run_avx512bw: $(ALL_AVX512BW)
	$(SDE) -- ./check_avx512bw

run_xop: $(ALL_XOP)
	./verify_xop
	./check_xop
	./speed_xop

VERIFY_DEPS=verify.cpp lookup.*.cpp unpack.*.cpp encode.*.cpp config.h

verify: $(VERIFY_DEPS)
	$(CXX) $(FLAGS) verify.cpp -o $@

verify_avx2: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX2) verify.cpp -o $@

verify_avx512: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512) verify.cpp -o $@

verify_avx512bw: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512BW) verify.cpp -o $@

verify_xop: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_XOP) verify.cpp -o $@


SPEED_DEPS=speed.cpp lookup.sse.cpp encode.*.cpp unpack.*.cpp functions.cpp application.cpp config.h $(EXT_DEPS)

speed: $(SPEED_DEPS)
	$(CXX) $(FLAGS) speed.cpp -o $@

speed_avx2: $(SPEED_DEPS) *.avx2.cpp
	$(CXX) $(FLAGS_AVX2) speed.cpp -o $@

speed_avx512: $(SPEED_DEPS) *.avx512.cpp
	$(CXX) $(FLAGS_AVX512) speed.cpp -o $@

speed_xop: $(SPEED_DEPS) *.xop.cpp
	$(CXX) $(FLAGS_XOP) speed.cpp -o $@


CHECK_DEPS=check.cpp lookup.sse.cpp encode.*.cpp unpack.*.cpp functions.cpp application.cpp config.h $(EXT_DEPS)

check: $(CHECK_DEPS)
	$(CXX) $(FLAGS) check.cpp -o $@

check_avx2: $(CHECK_DEPS) *.avx2.cpp
	$(CXX) $(FLAGS_AVX2) check.cpp -o $@

check_avx512: $(CHECK_DEPS) *.avx512.cpp
	$(CXX) $(FLAGS_AVX512) check.cpp -o $@

check_avx512bw: $(CHECK_DEPS)
	$(CXX) $(FLAGS_AVX512BW) check.cpp -o $@

check_xop: $(CHECK_DEPS) *.xop.cpp
	$(CXX) $(FLAGS_XOP) check.cpp -o $@

build_all: $(ALL_TARGETS)
verify_all: verify verify_avx2 verify_avx512
	# only Intel targets
	sde -cnl -- ./verify
	sde -cnl -- ./verify_avx2
	sde -cnl -- ./verify_avx512

unpack_test: unpack_bigendian.cpp
	$(CXX) $(FLAGS) $^ -o $@

clean:
	rm -f $(ALL_TARGETS)

