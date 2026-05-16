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
./main compress   <input>  [output]   # default output: <input>.bwtc
./main decompress <input>  [output]   # default output: <input>.out
```

Example:

```sh
./main compress   test_cases/sample.txt
./main decompress test_cases/sample.txt.bwtc  recovered.txt
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
