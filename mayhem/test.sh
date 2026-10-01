#!/usr/bin/env bash
#
# mayhem/test.sh — run the behavioral oracle built by mayhem/build.sh.
#
# NOTE: this suite is AUTHORED (known-answer oracle), not upstream's. Upstream ships no
# pass/fail test suite — src/bench.c is a benchmark whose sorted-order/validation checks
# only print and always exit 0. The oracle (mayhem/fluxsort-test.c) sorts many
# pattern/size combinations with fluxsort, fluxsort_size, quadsort and quadsort_size and
# asserts the output equals a qsort-derived reference, plus stability checks.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

RUNNER=/mayhem/fluxsort-test
if [ ! -x "$RUNNER" ]; then
  echo "FATAL: $RUNNER missing — mayhem/build.sh should have built it" >&2
  emit_ctrf "authored-known-answer" 0 1
  exit 1
fi

OUT="$("$RUNNER" 2>&1)"; RC=$?
echo "$OUT"
PASSED=$(echo "$OUT" | sed -n 's/^PASSED \([0-9]*\) FAILED [0-9]*$/\1/p')
FAILED=$(echo "$OUT" | sed -n 's/^PASSED [0-9]* FAILED \([0-9]*\)$/\1/p')
if [ -z "$PASSED" ] || [ -z "$FAILED" ]; then
  # runner crashed before printing its summary — that is a failure
  PASSED=0; FAILED=1
fi
[ "$RC" -ne 0 ] && [ "$FAILED" -eq 0 ] && FAILED=1
emit_ctrf "authored-known-answer" "$PASSED" "$FAILED"
