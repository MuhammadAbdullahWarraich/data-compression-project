#!/usr/bin/env bash
# Compress + decompress every file in test_cases/ and verify byte-for-byte
# match against the original. Exits non-zero on any mismatch.

set -u
PROG="./main"
DIR="./test_cases"

if [[ ! -x "$PROG" ]]; then
    echo "error: $PROG not found. Run 'make' first." >&2
    exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail=0
for f in "$DIR"/*; do
    [[ -f "$f" ]] || continue
    base=$(basename "$f")
    "$PROG" compress   "$f" "$TMP/$base.bwtc" >/dev/null
    "$PROG" decompress "$TMP/$base.bwtc" "$TMP/$base.out" >/dev/null
    if cmp -s "$f" "$TMP/$base.out"; then
        printf "  PASS  %s\n" "$base"
    else
        printf "  FAIL  %s\n" "$base"
        fail=1
    fi
done
exit $fail
