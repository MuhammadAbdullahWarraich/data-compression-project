#!/usr/bin/env bash
# For each Canterbury file (smallest first), sweep block sizes 100k..900k
# and compare our compressor against bzip2 at the same block size.
#
# Our compressor reads block_size from config.ini; bzip2 takes -1..-9.
# config.ini is edited in place per iteration and restored on exit.
#
# Usage: scripts/benchmark_per_file.sh [corpus-dir]
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
    local bs="$1"
    awk -v bs="$bs" '
        /^[[:space:]]*block_size[[:space:]]*=/ { print "block_size = " bs; next }
        { print }
    ' "$CFG_BACKUP" > "$CFG"
}

# Files sorted by size ascending (smallest first).
mapfile -t FILES < <(find "$CORPUS" -maxdepth 1 -type f -printf "%s %p\n" \
    | sort -n | awk '{print $2}')
if (( ${#FILES[@]} == 0 )); then
    echo "error: no files in $CORPUS" >&2
    exit 1
fi

BLOCK_STEPS=(1 2 3 4 5 6 7 8 9)

for f in "${FILES[@]}"; do
    base=$(basename "$f")
    orig=$(stat -c %s "$f")

    echo
    printf "########## %s (%d bytes) ##########\n" "$base" "$orig"
    printf "%-10s %12s %8s %8s %12s %8s %8s %10s\n" \
        "bs" "ours(B)" "ratio" "t(s)" "bz2(B)" "ratio" "t(s)" "roundtrip"
    printf -- '-%.0s' {1..90}; printf "\n"

    for step in "${BLOCK_STEPS[@]}"; do
        bs=$(( step * 100000 ))
        set_block_size "$bs"

        ours_out="$TMP/$base.bwtc"
        ours_t=$(timer "$PROG" compress "$f" "$ours_out")
        ours_sz=$(stat -c %s "$ours_out" 2>/dev/null || echo 0)
        if (( orig > 0 )); then
            ours_r=$(awk "BEGIN{printf \"%.4f\", $ours_sz/$orig}")
        else
            ours_r="-"
        fi

        cp "$f" "$TMP/$base.in"
        bz_t=$(timer bzip2 -k -f "-$step" "$TMP/$base.in")
        bz_sz=$(stat -c %s "$TMP/$base.in.bz2" 2>/dev/null || echo 0)
        if (( orig > 0 )); then
            bz_r=$(awk "BEGIN{printf \"%.4f\", $bz_sz/$orig}")
        else
            bz_r="-"
        fi

        # Roundtrip
        "$PROG" decompress "$ours_out" "$TMP/$base.rt" >/dev/null 2>&1
        if cmp -s "$f" "$TMP/$base.rt"; then
            rt="PASS"
        else
            rt="FAIL"
        fi

        printf "%-10d %12d %8s %8s %12d %8s %8s %10s\n" \
            "$bs" "$ours_sz" "$ours_r" "$ours_t" "$bz_sz" "$bz_r" "$bz_t" "$rt"

        rm -f "$TMP/$base.in" "$TMP/$base.in.bz2" "$ours_out" "$TMP/$base.rt"
    done
done
