.SUFFIXES:
.PHONY: all clean run

SDE=sde -cnl
FLAGS_BASE=$(CXXFLAGS) -Wall -Wextra -pedantic -O3 -std=c++14

HAVE_BMI2=$(shell python script/cpuflags.py bmi2)
ifeq ($(HAVE_BMI2),present)
    FLAGS_BASE+=-mbmi2 -DHAVE_BMI2_INSTRUCTIONS
endif

FLAGS=$(FLAGS_BASE) -msse4 -DHAVE_SSE_INSTRUCTIONS
FLAGS_AVX2=$(FLAGS_BASE) -mavx2 -mbmi2 -DHAVE_AVX2_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512=$(FLAGS_BASE) -mavx512f -mbmi2 -DHAVE_AVX512_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS
FLAGS_AVX512BW=$(FLAGS_BASE) -mavx512bw -mbmi2 -DHAVE_AVX512BW_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS 
FLAGS_AVX512VBMI=$(FLAGS_BASE) -mavx512vbmi -mbmi2 -DHAVE_AVX512VBMI_INSTRUCTIONS -DHAVE_BMI2_INSTRUCTIONS 
FLAGS_AVX512VL=$(FLAGS_AVX512VBMI) -DHAVE_AVX512VL_INSTRUCTIONS
FLAGS_XOP=$(FLAGS_BASE) -mxop -DHAVE_XOP_INSTRUCTIONS
FLAGS_ARM=$(FLAGS_BASE) -mfpu=neon -DHAVE_NEON_INSTRUCTIONS
FLAGS_RVV=$(FLAGS_BASE) -march=rv64gcv -DHAVE_RVV_INSTRUCTIONS -static

DEPS=decode.*.cpp decoders.*.cpp lookup.*.cpp pack.*.cpp all.cpp function_registry.cpp ../avx512_swar.cpp ../config.h

ALL=verify speed check benchmark check_ws
ALL_XOP=verify_xop speed_xop check_xop benchmark_xop
ALL_ARM=verify_arm speed_arm check_arm
ALL_AVX2=verify_avx2 speed_avx2 check_avx2 benchmark_avx2
ALL_AVX512=verify_avx512 check_avx512 speed_avx512 benchmark_avx512
ALL_AVX512BW=verify_avx512bw check_avx512bw speed_avx512bw benchmark_avx512bw
ALL_AVX512VBMI=verify_avx512vbmi check_avx512vbmi benchmark_avx512vbmi
ALL_RVV=verify_rvv check_rvv check_ws_rvv benchmark_rvv
ALL_INTEL_TARGETS=$(ALL) $(ALL_XOP) $(ALL_AVX2) $(ALL_AVX512) $(ALL_AVX512VBMI) $(ALL_AVX512BW)
ALL_TARGETS=$(ALL_INTEL_TARGETS) $(ALL_ARM) $(ALL_RVV)

all: $(ALL)

sse: $(ALL)

xop: $(ALL_XOP)

arm: $(ALL_ARM)

avx2: $(ALL_AVX2)

avx512: $(ALL_AVX512)

avx512bw: $(ALL_AVX512BW)

avx512vbmi: $(ALL_AVX512VBMI)

run: $(ALL)
	./verify
	./check
	./speed

run_sse: run

run_xop: $(ALL_XOP)
	./verify_xop
	./check_xop
	./speed_xop

run_avx2: $(ALL_AVX2)
	./verify_avx2
	./check_avx2
	./speed_avx2

run_avx512: $(ALL_AVX512)
	$(SDE) -- ./verify_avx512
	$(SDE) -- ./check_avx512
	$(SDE) -- ./speed_avx512

run_avx512bw: $(ALL_AVX512BW)
	$(SDE) -- ./verify_avx512bw
	$(SDE) -- ./check_avx512bw

run_avx512vbmi: $(ALL_AVX512VBMI)
	$(SDE) -- ./verify_avx512vbmi
	$(SDE) -- ./check_avx512vbmi

VERIFY_DEPS=verify.cpp $(DEPS)

verify: $(VERIFY_DEPS)
	$(CXX) $(FLAGS) $< -o $@

verify_xop: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_XOP) $< -o $@

verify_avx2: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX2) $< -o $@

verify_avx512: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512) $< -o $@

verify_avx512vbmi: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

verify_avx512bw: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

verify_arm: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_ARM) $< -o $@

verify_rvv: $(VERIFY_DEPS)
	$(CXX) $(FLAGS_RVV) $< -o $@


SPEED_DEPS=speed.cpp $(DEPS) application.cpp ../cmdline.cpp ../gettime.cpp

speed: $(SPEED_DEPS)
	$(CXX) $(FLAGS) $< -o $@

speed_xop: $(SPEED_DEPS)
	$(CXX) $(FLAGS_XOP) $< -o $@

speed_avx2: $(SPEED_DEPS)
	$(CXX) $(FLAGS_AVX2) $< -o $@

speed_avx512: $(SPEED_DEPS)
	$(CXX) $(FLAGS_AVX512) $< -o $@

speed_avx512bw: $(SPEED_DEPS)
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

speed_arm: $(SPEED_DEPS)
	$(CXX) $(FLAGS_ARM) $< -o $@


CHECK_DEPS=check.cpp $(DEPS) application.cpp ../cmdline.cpp ../fnv32.cpp

check: $(CHECK_DEPS)
	$(CXX) $(FLAGS) $< -o $@

check_xop: $(CHECK_DEPS)
	$(CXX) $(FLAGS_XOP) $< -o $@

check_avx2: $(CHECK_DEPS)
	$(CXX) $(FLAGS_AVX2) $< -o $@

check_avx512: $(CHECK_DEPS)
	$(CXX) $(FLAGS_AVX512) $< -o $@

check_avx512bw: $(CHECK_DEPS)
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

check_avx512vbmi: $(CHECK_DEPS)
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

check_arm: $(CHECK_DEPS)
	$(CXX) $(FLAGS_ARM) $< -o $@

check_rvv: $(CHECK_DEPS)
	$(CXX) $(FLAGS_RVV) -DBUFFER_SIZE="1*1024" -DITERATIONS=1 $< -o $@


CHECK_WS_DEPS=check_ws.cpp $(DEPS) ../cmdline.cpp ../fnv32.cpp

check_ws: $(CHECK_WS_DEPS)
	$(CXX) $(FLAGS) $< -o $@

check_ws_rvv: $(CHECK_WS_DEPS)
	$(CXX) $(FLAGS_RVV) $< -o $@


BENCHMARK_DEPS=benchmark.cpp $(DEPS) application.cpp ../cmdline.cpp ../benchmark.h ../memcpy.cpp

benchmark: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS) $< -o $@

benchmark_xop: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_XOP) $< -o $@

benchmark_avx2: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_AVX2) $< -o $@

benchmark_avx512: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_AVX512) $< -o $@

benchmark_avx512bw: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_AVX512BW) $< -o $@

benchmark_avx512vbmi: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_AVX512VBMI) $< -o $@

benchmark_avx512vl: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_AVX512VL) $< -o $@

benchmark_rvv: $(BENCHMARK_DEPS)
	$(CXX) $(FLAGS_RVV) -DBUFFER_SIZE="4 * 1024" -DITERATIONS=100 $< -o $@

build_intel: $(ALL_INTEL_TARGETS)
verify_intel: verify verify_avx2 verify_avx512 verify_avx512bw
	sde -cnl -- ./verify
	sde -cnl -- ./verify_avx2
	sde -cnl -- ./verify_avx512
	sde -cnl -- ./verify_avx512bw

SPIKE_ISA=rv64gcv_Zicntr

verify_riscv: verify_rvv
	#spike -l --log-commits --log=log --isa=$(SPIKE_ISA) `which pk` $^
	spike --isa=$(SPIKE_ISA) `which pk` $^

check_riscv: check_rvv
	spike --isa=$(SPIKE_ISA) `which pk` $^

check_ws_riscv: check_ws_rvv
	spike --isa=$(SPIKE_ISA) `which pk` $^

benchmark_riscv: benchmark_rvv
	spike --isa=$(SPIKE_ISA) `which pk` $^

clean:
	rm -f $(ALL_TARGETS)
