# BWT-based Compressor

A Burrows–Wheeler-transform compression pipeline in C:

```
file → block split → RLE1 → BWT → MTF → RLE2 → Huffman → .bwtc
```

Each stage can be toggled in `config.ini`. The BWT variant (`matrix` or
`suffix_array`) is also chosen there.

## Build

```sh
make
```

Produces `./main`. Object files land in `build/`.

## Usage

`config.ini` is read from the current working directory.

```sh
./main [-c|-d] <input> [output]

  -c   compress (default if no flag is given)
  -d   decompress
```

If `[output]` is omitted, the result is written to the current working
directory using the input's basename plus `.bwtc` (compress) or `.out`
(decompress). For example, `./main -c test_cases/small.txt` produces
`./small.txt.bwtc`, not `test_cases/small.txt.bwtc`.

Examples:

```sh
./main test_cases/sample.txt                       # compress (default) → ./sample.txt.bwtc
./main -c test_cases/sample.txt                    # same, explicit
./main -c test_cases/sample.txt out.bwtc           # custom output path
./main -d sample.txt.bwtc                          # decompress → ./sample.txt.bwtc.out
./main -d sample.txt.bwtc recovered.txt            # decompress to a chosen path
diff test_cases/sample.txt recovered.txt
```

## Tests and benchmarks

```sh
make test       # roundtrip check on every file in test_cases/
make benchmark  # side-by-side comparison with bzip2 on benchmarks/
```

## Project layout

```
include/        public headers
src/            implementation (block, bwt, rle, rle2, mtf, huffman, compressor)
main.c          CLI driver
config.ini      pipeline configuration
test_cases/     small inputs for the roundtrip test
benchmarks/     larger inputs for performance comparison
scripts/        test and benchmark drivers
third-party/    inih (config parser)
```

## File format

```
[ magic "BWTC" | version u8 | flags u8 | block_size u32 | num_blocks u32 ]
per block:
  [ orig_size u32 | primary_index u32 | mtf_char_count u32
    | mtf_sorted_chars[mtf_char_count] | payload_size u32
    | payload[payload_size] ]
```

The `flags` byte records which pipeline stages and which BWT variant were used,
so the decoder is self-describing.

## Benchmark snapshot

Block size 10000, all stages enabled, suffix-array BWT. Times measured on a
single run.

| File             | Original   | Ours (B) | Ratio   | Time (s) | bzip2 (B) | Ratio   | Time (s) |
|------------------|-----------:|---------:|--------:|---------:|----------:|--------:|---------:|
| big_text.txt     | 200,000    | 1,236    | 0.0062  | 30.13    | 124       | 0.0006  | 0.19     |
| medium100k.txt   | 100,000    | 793      | 0.0079  | 6.21     | 121       | 0.0012  | 0.02     |
