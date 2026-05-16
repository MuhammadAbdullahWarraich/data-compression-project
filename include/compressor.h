#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Pipeline configuration. Mirrors fields parsed from config.ini that
 * actually affect the encoder/decoder. Output paths and benchmarking
 * options live elsewhere.
 */
typedef struct {
    unsigned int block_size;
    bool rle1_enabled;
    bool mtf_enabled;
    bool rle2_enabled;
    bool huffman_enabled;
    bool bwt_suffix_array;   // true => bwt_encode_meta, false => bwt_encode
} CompressorConfig;

/*
 * Run the full forward pipeline on `input_path` and write the compressed
 * stream to `output_path`. Returns 0 on success, non-zero on failure.
 */
int compressor_compress(const char *input_path,
                        const char *output_path,
                        const CompressorConfig *cfg);

/*
 * Run the inverse pipeline. Reads the compressed file's self-describing
 * header to figure out which stages were applied. Returns 0 on success.
 */
int compressor_decompress(const char *input_path,
                          const char *output_path);

#endif // COMPRESSOR_H
