.SUFFIXES:
.PHONY: all avx2 avx512 avx512bw avx512vbmi avx512vl clean

SDE=sde -cnl --
FLAGS_BASE=$(CXXFLAGS) -Wall -Wextra -pedantic -O3

HAVE_BMI2=$(shell python script/cpuflags.py bmi2)
ifeq ($(HAVE_BMI2),present)
    FLAGS_BASE+=-mbmi2 -DHAVE_BMI2_INSTRUCTIONS
endif

FLAGS=$(FLAGS_BASE) -std=c++11 -msse4 -DHAVE_SSE_INSTRUCTIONS
FLAGS_AVX2=$(FLAGS_BASE) -std=c++11 -mavx2 -mbmi2 -DHAVE_AVX2_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512=$(FLAGS_BASE) -std=c++11 -mavx512f -mbmi2 -DHAVE_AVX512_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512BW=$(FLAGS_BASE) -std=c++11 -mavx512bw -mbmi2 -DHAVE_AVX512BW_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512VBMI=$(FLAGS_BASE) -std=c++11 -mavx512vbmi -mbmi2 -DHAVE_AVX512VBMI_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512VL=$(FLAGS_AVX512VBMI) -DHAVE_AVX512VL_INSTRUCTIONS
FLAGS_XOP=$(FLAGS_BASE) -std=c++11 -mxop -DHAVE_XOP_INSTRUCTIONS
FLAGS_ARM=$(FLAGS_BASE) -std=gnu++11 -mfpu=neon -DHAVE_NEON_INSTRUCTIONS
FLAGS_RVV=$(FLAGS_BASE) -std=c++11 -march=rv64gcv -DHAVE_RVV_INSTRUCTIONS -static

EXT_DEPS=../fnv32.cpp ../gettime.cpp

ALL=verify speed check benchmark
ALL_AVX2=verify_avx2 speed_avx2 check_avx2 benchmark_avx2
ALL_AVX512=verify_avx512 check_avx512 speed_avx512 benchmark_avx512
ALL_AVX512BW=verify_avx512bw check_avx512bw speed_avx512bw benchmark_avx512bw
ALL_AVX512VBMI=verify_avx512vbmi check_avx512vbmi benchmark_avx512vbmi
ALL_AVX512VL=verify_avx512vl check_avx512vl benchmark_avx512vl
ALL_XOP=verify_xop speed_xop check_xop benchmark_xop
ALL_ARM=verify_arm speed_arm check_arm
ALL_RVV=verify_rvv speed_rvv check_rvv
ALL_INTEL_TARGETS=$(ALL) $(ALL_AVX2) $(ALL_AVX512) $(ALL_AVX512BW) $(ALL_AVX512VBMI) $(ALL_AVX512VL) $(ALL_XOP)
ALL_TARGETS=$(ALL_INTEL_TARGETS) $(ALL_ARM) $(ALL_RVV)

all: $(ALL)

sse: $(ALL)

avx2: $(ALL_AVX2)

avx512: $(ALL_AVX512)

avx512bw: $(ALL_AVX512BW)

avx512vbmi: $(ALL_AVX512VBMI)

avx512vl: $(ALL_AVX512VL)

xop: $(ALL_XOP)

arm: $(ALL_ARM)

run: $(ALL)
	./verify
	./check
	./speed

run_sse: run

run_avx2: $(ALL_AVX2)
	./verify_avx2
	./check_avx2
	./speed_avx2

run_avx512: $(ALL_AVX512)
	$(SDE) ./verify_avx512
	$(SDE) ./check_avx512

run_avx512bw: $(ALL_AVX512)
	$(SDE) ./verify_avx512
	$(SDE) ./check_avx512

run_avx512vbmi: $(ALL_AVX512VBMI)
	$(SDE) ./verify_avx512vbmi
	$(SDE) ./check_avx512vbmi

run_avx512vl: $(ALL_AVX512VL)
	$(SDE) ./verify_avx512vl
	$(SDE) ./check_avx512vl

run_xop: $(ALL_XOP)
	./verify_xop
	./check_xop
	./speed_xop

VERIFY_DEPS=verify.cpp lookup.*.cpp unpack.*.cpp encode.*.cpp ../config.h

verify: $(VERIFY_DEPS)
	$(CXX) $(FLAGS) $< -o $@

verify_avx2: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX2) $< -o $@

verify_avx512: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512) $< -o $@

verify_avx512bw: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

verify_avx512vbmi: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

verify_avx512vl: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512VL) $< -o $@

verify_xop: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_XOP) $< -o $@

verify_arm: $(VERIFY_DEPS) *.neon.cpp
	$(CXX) $(FLAGS_ARM) $< -o $@

verify_rvv: $(VERIFY_DEPS) *.rvv.cpp ../rvv-debug.h
	$(CXX) $(FLAGS_RVV) $< -o $@

APPLICATION_DEPS=application.cpp ../config.h

SPEED_DEPS=speed.cpp lookup.sse.cpp encode.*.cpp unpack.*.cpp $(APPLICATION_DEPS) $(EXT_DEPS)

speed: $(SPEED_DEPS)
	$(CXX) $(FLAGS) $< -o $@

speed_avx2: $(SPEED_DEPS) *.avx2.cpp
	$(CXX) $(FLAGS_AVX2) $< -o $@

speed_avx512: $(SPEED_DEPS) *.avx512.cpp
	$(CXX) $(FLAGS_AVX512) $< -o $@

speed_avx512bw: $(SPEED_DEPS) *.avx512.cpp *.avx512bw.cpp
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

speed_avx512vbmi: $(SPEED_DEPS) *.avx512.cpp *.avx512bw.cpp *.avx512vbmi.cpp
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

speed_avx512vl: $(SPEED_DEPS) *.avx512.cpp *.avx512bw.cpp *.avx512vbmi.cpp
	$(CXX) $(FLAGS_AVX512VL) $< -o $@

speed_xop: $(SPEED_DEPS) *.xop.cpp
	$(CXX) $(FLAGS_XOP) $< -o $@

speed_arm: $(SPEED_DEPS)
	$(CXX) $(FLAGS_ARM) $< -o $@

speed_rvv: $(SPEED_DEPS) *.rvv.cpp
	$(CXX) $(FLAGS_RVV) -DBUFFER_SIZE=8912 $< -o $@


CHECK_DEPS=check.cpp lookup.sse.cpp encode.*.cpp unpack.*.cpp $(APPLICATION_DEPS) $(EXT_DEPS)

check: $(CHECK_DEPS)
	$(CXX) $(FLAGS) $< -o $@

check_avx2: $(CHECK_DEPS) *.avx2.cpp
	$(CXX) $(FLAGS_AVX2) $< -o $@

check_avx512: $(CHECK_DEPS) *.avx512.cpp
	$(CXX) $(FLAGS_AVX512) $< -o $@

check_avx512bw: $(CHECK_DEPS) *.avx512.cpp *.avx512bw.cpp
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

check_avx512vbmi: $(CHECK_DEPS) *.avx512.cpp *.avx512bw.cpp *.avx512vbmi.cpp
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

check_avx512vl: $(CHECK_DEPS) *.avx512.cpp *.avx512bw.cpp *.avx512vl.cpp
	$(CXX) $(FLAGS_AVX512VL) $< -o $@

check_xop: $(CHECK_DEPS) *.xop.cpp
	$(CXX) $(FLAGS_XOP) $< -o $@

check_arm: $(CHECK_DEPS)
	$(CXX) $(FLAGS_ARM) -DBUFFER_SIZE=4194304 $< -o $@

check_rvv: $(CHECK_DEPS) *.rvv.cpp
	$(CXX) $(FLAGS_RVV) -DBUFFER_SIZE=262144 $< -o $@


BENCHMARK_DEPS=benchmark.cpp lookup.sse.cpp encode.*.cpp unpack.*.cpp $(APPLICATION_DEPS) ../benchmark.h

benchmark: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS) $< -o $@

benchmark_xop: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_XOP) $< -o $@

benchmark_avx2: $(BENCHMARK_DEPS) *.avx2.cpp
	$(CXX) $(FLAGS_AVX2) $< -o $@

benchmark_avx512: $(BENCHMARK_DEPS) *.avx512.cpp
	$(CXX) $(FLAGS_AVX512) $< -o $@

benchmark_avx512bw: $(BENCHMARK_DEPS) *.avx512.cpp *.avx512bw.cpp
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

benchmark_avx512vbmi: $(BENCHMARK_DEPS) *.avx512.cpp *.avx512bw.cpp *.avx512vbmi.cpp
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

benchmark_avx512vl: $(BENCHMARK_DEPS) *.avx512.cpp *.avx512bw.cpp *.avx512vl.cpp
	$(CXX) $(FLAGS_AVX512VL) $< -o $@


build_intel: $(ALL_INTEL_TARGETS)

verify_intel: verify verify_avx2 verify_avx512 verify_avx512bw verify_avx512vbmi verify_avx512vl
	$(SDE) ./verify
	$(SDE) ./verify_avx2
	$(SDE) ./verify_avx512
	$(SDE) ./verify_avx512bw
	$(SDE) ./verify_avx512vbmi
	$(SDE) ./verify_avx512vl

SPIKE_ISA=rv64gcv_Zicntr

verify_riscv: verify_rvv
	#spike -l --log-commits --log=log --isa=$(SPIKE_ISA) `which pk` $^
	spike --isa=$(SPIKE_ISA) `which pk` $^

check_riscv: check_rvv
	spike --isa=$(SPIKE_ISA) `which pk` $^

speed_riscv: speed_rvv
	spike --isa=$(SPIKE_ISA) `which pk` $^

unpack_test: unpack_bigendian.cpp
	$(CXX) $(FLAGS) $^ -o $@

clean:
	$(RM) $(ALL_TARGETS)

