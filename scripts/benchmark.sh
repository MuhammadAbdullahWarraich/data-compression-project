#!/usr/bin/env bash
# Compare our compressor against bzip2 on a corpus of files.
# Usage: scripts/benchmark.sh [corpus-dir]
# Default corpus-dir: ./benchmarks (falls back to ./test_cases if empty)

set -u

PROG="./main"
CORPUS="${1:-./benchmarks}"

if [[ ! -x "$PROG" ]]; then
    echo "error: $PROG not found. Run 'make' first." >&2
    exit 1
fi

if [[ ! -d "$CORPUS" ]] || [[ -z "$(ls -A "$CORPUS" 2>/dev/null)" ]]; then
    echo "info: '$CORPUS' empty or missing, falling back to ./test_cases" >&2
    CORPUS="./test_cases"
fi

if ! command -v bzip2 >/dev/null 2>&1; then
    echo "error: bzip2 not installed" >&2
    exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

timer() {
    # Print real seconds elapsed for the command using date +%s.%N.
    local start end
    start=$(date +%s.%N)
    "$@" > /dev/null 2>&1
    end=$(date +%s.%N)
    awk -v s="$start" -v e="$end" 'BEGIN{printf "%.3f", e - s}'
}

printf "%-30s %12s %16s %10s %8s %16s %10s %8s\n" \
    "file" "orig(B)" "ours(B)" "ratio" "time(s)" "bzip2(B)" "ratio" "time(s)"
printf -- '-%.0s' {1..130}; printf "\n"

for f in "$CORPUS"/*; do
    [[ -f "$f" ]] || continue
    base=$(basename "$f")
    orig=$(stat -c %s "$f")

    # Ours
    ours_out="$TMP/$base.bwtc"
    ours_time=$(timer "$PROG" compress "$f" "$ours_out")
    ours_size=$(stat -c %s "$ours_out" 2>/dev/null || echo 0)
    ours_ratio=$(awk "BEGIN{printf \"%.4f\", $ours_size/$orig}")

    # bzip2
    cp "$f" "$TMP/$base.bz2.in"
    bz_time=$(timer bzip2 -k -f "$TMP/$base.bz2.in")
    bz_size=$(stat -c %s "$TMP/$base.bz2.in.bz2" 2>/dev/null || echo 0)
    bz_ratio=$(awk "BEGIN{printf \"%.4f\", $bz_size/$orig}")

    printf "%-30s %12d %16d %10s %8s %16d %10s %8s\n" \
        "$base" "$orig" "$ours_size" "$ours_ratio" "$ours_time" \
        "$bz_size" "$bz_ratio" "$bz_time"

    # Verify roundtrip
    rt_out="$TMP/$base.rt"
    "$PROG" decompress "$ours_out" "$rt_out" > /dev/null 2>&1
    if ! cmp -s "$f" "$rt_out"; then
        echo "  ! ROUNDTRIP FAILED for $base" >&2
    fi
done
