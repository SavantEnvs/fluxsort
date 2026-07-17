#!/usr/bin/env bash
#
# mayhem/build.sh — build the fluxsort fuzz harness, its standalone reproducer, and the
# behavioral test oracle. fluxsort is a header-only library (src/fluxsort.h includes
# fluxsort.c / quadsort.c), so each binary is a single-TU compile with -Isrc; there is
# no separate library build step — the library code is compiled (and thus sanitized /
# instrumented) directly into the fuzzer.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# 1+2) Fuzzer + standalone reproducer (library code compiled into the harness TU,
#      so $SANITIZER_FLAGS instruments the fuzzed code itself).
$CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
    -I"$SRC/src" "$SRC/mayhem/fluxsort-fuzz.c" -o /mayhem/fluxsort-fuzz -lm

$CC $SANITIZER_FLAGS $DEBUG_FLAGS "$STANDALONE_FUZZ_MAIN" \
    -I"$SRC/src" "$SRC/mayhem/fluxsort-fuzz.c" -o /mayhem/fluxsort-fuzz-standalone -lm

# 3) Behavioral test oracle (normal flags, no sanitizers) — run by mayhem/test.sh.
$CC -O2 $COVERAGE_FLAGS -I"$SRC/src" "$SRC/mayhem/fluxsort-test.c" -o /mayhem/fluxsort-test -lm
