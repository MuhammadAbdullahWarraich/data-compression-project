#!/usr/bin/env bash
# Compare our compressor vs bzip2 across block sizes 100k..900k on the
# Canterbury corpus.
#
# Our compressor reads block_size from config.ini; bzip2 takes -1..-9.
# This script edits config.ini in-place per iteration and restores the
# original on exit.
#
# Usage: scripts/benchmark_block_sizes.sh [corpus-dir]
# Default corpus-dir: ./test_cases/canterbury

set -u

PROG="./main"
CORPUS="${1:-./test_cases/canterbury}"
CFG="./config.ini"

if [[ ! -x "$PROG" ]]; then
    echo "error: $PROG not found. Run 'make' first." >&2
    exit 1
fi
if [[ ! -d "$CORPUS" ]]; then
    echo "error: corpus dir '$CORPUS' not found" >&2
    exit 1
fi
if [[ ! -f "$CFG" ]]; then
    echo "error: $CFG not found" >&2
    exit 1
fi
if ! command -v bzip2 >/dev/null 2>&1; then
    echo "error: bzip2 not installed" >&2
    exit 1
fi

CFG_BACKUP="$(mktemp)"
cp "$CFG" "$CFG_BACKUP"
TMP="$(mktemp -d)"
trap 'cp "$CFG_BACKUP" "$CFG"; rm -f "$CFG_BACKUP"; rm -rf "$TMP"' EXIT

timer() {
    local start end
    start=$(date +%s.%N)
    "$@" > /dev/null 2>&1
    end=$(date +%s.%N)
    awk -v s="$start" -v e="$end" 'BEGIN{printf "%.3f", e - s}'
}

set_block_size() {
    # Replace the block_size line in config.ini.
    local bs="$1"
    awk -v bs="$bs" '
        /^[[:space:]]*block_size[[:space:]]*=/ { print "block_size = " bs; next }
        { print }
    ' "$CFG_BACKUP" > "$CFG"
}

# Collect files (sorted, regular only), excluding the two slowest binaries.
SKIP_RE='^(kennedy\.xls|ptt5)$'
mapfile -t FILES < <(find "$CORPUS" -maxdepth 1 -type f \
    | while read -r p; do
        b=$(basename "$p")
        if [[ ! "$b" =~ $SKIP_RE ]]; then echo "$p"; fi
      done | sort)
if (( ${#FILES[@]} == 0 )); then
    echo "error: no files in $CORPUS" >&2
    exit 1
fi

# Block sizes (KB): 100, 300, 500, 700, 900. bzip2 flag = bs / 100000.
BLOCK_STEPS=(1 3 5 7 9)

# Summary accumulators keyed by step value.
declare -A SUM_ORIG SUM_OURS SUM_BZ SUM_T_OURS SUM_T_BZ
for step in "${BLOCK_STEPS[@]}"; do
    SUM_ORIG[$step]=0; SUM_OURS[$step]=0; SUM_BZ[$step]=0
    SUM_T_OURS[$step]=0; SUM_T_BZ[$step]=0
done

for step in "${BLOCK_STEPS[@]}"; do
    bs=$(( step * 100000 ))
    set_block_size "$bs"

    echo
    printf "===== block_size = %d (bzip2 -%d) =====\n" "$bs" "$step"
    printf "%-18s %10s %12s %8s %8s %12s %8s %8s\n" \
        "file" "orig(B)" "ours(B)" "ratio" "t(s)" "bz2(B)" "ratio" "t(s)"
    printf -- '-%.0s' {1..96}; printf "\n"

    for f in "${FILES[@]}"; do
        base=$(basename "$f")
        orig=$(stat -c %s "$f")

        ours_out="$TMP/$base.bwtc"
        ours_t=$(timer "$PROG" compress "$f" "$ours_out")
        ours_sz=$(stat -c %s "$ours_out" 2>/dev/null || echo 0)
        ours_r=$(awk "BEGIN{printf \"%.4f\", $ours_sz/$orig}")

        # bzip2: copy input so the .bz2 lands in tmp; -N selects block size
        cp "$f" "$TMP/$base.in"
        bz_t=$(timer bzip2 -k -f "-$step" "$TMP/$base.in")
        bz_sz=$(stat -c %s "$TMP/$base.in.bz2" 2>/dev/null || echo 0)
        bz_r=$(awk "BEGIN{printf \"%.4f\", $bz_sz/$orig}")

        printf "%-18s %10d %12d %8s %8s %12d %8s %8s\n" \
            "$base" "$orig" "$ours_sz" "$ours_r" "$ours_t" \
            "$bz_sz" "$bz_r" "$bz_t"

        # Roundtrip check
        "$PROG" decompress "$ours_out" "$TMP/$base.rt" >/dev/null 2>&1
        if ! cmp -s "$f" "$TMP/$base.rt"; then
            echo "  ! ROUNDTRIP FAILED for $base at bs=$bs" >&2
        fi

        SUM_ORIG[$step]=$(( ${SUM_ORIG[$step]} + orig ))
        SUM_OURS[$step]=$(( ${SUM_OURS[$step]} + ours_sz ))
        SUM_BZ[$step]=$(( ${SUM_BZ[$step]} + bz_sz ))
        SUM_T_OURS[$step]=$(awk "BEGIN{printf \"%.3f\", ${SUM_T_OURS[$step]} + $ours_t}")
        SUM_T_BZ[$step]=$(awk "BEGIN{printf \"%.3f\", ${SUM_T_BZ[$step]} + $bz_t}")

        rm -f "$TMP/$base.in" "$TMP/$base.in.bz2" "$ours_out" "$TMP/$base.rt"
    done
done

echo
echo "===== SUMMARY (corpus totals per block size) ====="
printf "%-10s %12s %12s %8s %8s %12s %8s %8s\n" \
    "bs" "orig(B)" "ours(B)" "ratio" "t(s)" "bz2(B)" "ratio" "t(s)"
printf -- '-%.0s' {1..90}; printf "\n"
for step in "${BLOCK_STEPS[@]}"; do
    bs=$(( step * 100000 ))
    orig=${SUM_ORIG[$step]}
    ours=${SUM_OURS[$step]}
    bz=${SUM_BZ[$step]}
    our_r=$(awk "BEGIN{printf \"%.4f\", $ours/$orig}")
    bz_r=$(awk "BEGIN{printf \"%.4f\", $bz/$orig}")
    printf "%-10d %12d %12d %8s %8s %12d %8s %8s\n" \
        "$bs" "$orig" "$ours" "$our_r" "${SUM_T_OURS[$step]}" \
        "$bz" "$bz_r" "${SUM_T_BZ[$step]}"
done
