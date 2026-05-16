#include "compressor.h"
#include "block.h"
#include "bwt.h"
#include "huffman.h"
#include "mtf.h"
#include "rle.h"
#include "rle2.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_BYTES "BWTC"
#define FORMAT_VERSION 1

// Worst-case expansion through the pipeline (RLE1 ~3x, RLE2 ~2x,
// Huffman header 264). 8x + 4096 is comfortable headroom for any block.
static inline size_t work_buffer_size(size_t block_size) {
    return 8 * block_size + 4096;
}

static inline void swap_ptr(unsigned char **a, unsigned char **b) {
    unsigned char *t = *a; *a = *b; *b = t;
}

static int read_exact(FILE *f, void *dst, size_t n) {
    return fread(dst, 1, n, f) == n ? 0 : -1;
}

static int write_exact(FILE *f, const void *src, size_t n) {
    return fwrite(src, 1, n, f) == n ? 0 : -1;
}

int compressor_compress(const char *input_path,
                        const char *output_path,
                        const CompressorConfig *cfg) {
    BlockManager *mgr = divide_into_blocks(input_path, cfg->block_size);
    if (!mgr) {
        fprintf(stderr, "compressor: failed to read %s\n", input_path);
        return -1;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("compressor: fopen output");
        free_block_manager(mgr);
        return -1;
    }

    // -------- File header --------
    uint8_t version = FORMAT_VERSION;
    uint8_t flags = 0;
    if (cfg->rle1_enabled)     flags |= 0x01;
    if (cfg->mtf_enabled)      flags |= 0x02;
    if (cfg->rle2_enabled)     flags |= 0x04;
    if (cfg->huffman_enabled)  flags |= 0x08;
    if (cfg->bwt_suffix_array) flags |= 0x10;

    uint32_t block_size_u32 = cfg->block_size;
    uint32_t num_blocks_u32 = (uint32_t)mgr->num_blocks;

    write_exact(out, MAGIC_BYTES, 4);
    write_exact(out, &version, 1);
    write_exact(out, &flags, 1);
    write_exact(out, &block_size_u32, 4);
    write_exact(out, &num_blocks_u32, 4);

    // -------- Working buffers (ping-pong) --------
    size_t cap = work_buffer_size(cfg->block_size);
    unsigned char *buf_a = malloc(cap);
    unsigned char *buf_b = malloc(cap);
    if (!buf_a || !buf_b) {
        fprintf(stderr, "compressor: malloc failed\n");
        free(buf_a); free(buf_b);
        fclose(out);
        free_block_manager(mgr);
        return -1;
    }

    for (int i = 0; i < mgr->num_blocks; i++) {
        unsigned char *cur = buf_a;
        unsigned char *nxt = buf_b;
        size_t len = mgr->blocks[i].original_size;
        memcpy(cur, mgr->blocks[i].data, len);

        size_t primary_index = 0;
        size_t mtf_char_count = 0;
        unsigned char *mtf_sorted_chars = NULL;

        // 1) RLE1
        if (cfg->rle1_enabled) {
            size_t out_len = 0;
            rle1_encode(cur, len, nxt, &out_len);
            swap_ptr(&cur, &nxt);
            len = out_len;
        }

        // 2) BWT (always on)
        if (cfg->bwt_suffix_array) {
            bwt_encode_meta(cur, len, nxt, &primary_index);
        } else {
            bwt_encode(cur, len, nxt, &primary_index);
        }
        swap_ptr(&cur, &nxt);
        len += 1; // both BWT variants append a trailing sentinel byte

        // 3) MTF
        if (cfg->mtf_enabled) {
            mtf_encode(cur, len, nxt, &mtf_char_count, &mtf_sorted_chars);
            swap_ptr(&cur, &nxt);
        }

        // 4) RLE2
        if (cfg->rle2_enabled) {
            size_t out_len = 0;
            rle2_encode(cur, len, nxt, &out_len);
            swap_ptr(&cur, &nxt);
            len = out_len;
        }

        // 5) Huffman
        if (cfg->huffman_enabled) {
            size_t out_len = 0;
            huffman_encode(cur, len, nxt, &out_len);
            swap_ptr(&cur, &nxt);
            len = out_len;
        }

        mgr->blocks[i].primary_index = (int)primary_index;

        // -------- Per-block record --------
        uint32_t orig_size_u32   = (uint32_t)mgr->blocks[i].original_size;
        uint32_t primary_u32     = (uint32_t)primary_index;
        uint32_t mtf_count_u32   = (uint32_t)mtf_char_count;
        uint32_t payload_size_u32 = (uint32_t)len;

        write_exact(out, &orig_size_u32, 4);
        write_exact(out, &primary_u32, 4);
        write_exact(out, &mtf_count_u32, 4);
        if (mtf_char_count > 0 && mtf_sorted_chars) {
            write_exact(out, mtf_sorted_chars, mtf_char_count);
        }
        write_exact(out, &payload_size_u32, 4);
        write_exact(out, cur, len);

        free(mtf_sorted_chars);
    }

    free(buf_a);
    free(buf_b);
    fclose(out);
    free_block_manager(mgr);
    return 0;
}

int compressor_decompress(const char *input_path,
                          const char *output_path) {
    FILE *in = fopen(input_path, "rb");
    if (!in) {
        perror("compressor: fopen input");
        return -1;
    }

    char magic[4];
    uint8_t version, flags;
    uint32_t block_size_u32, num_blocks_u32;
    if (read_exact(in, magic, 4) ||
        memcmp(magic, MAGIC_BYTES, 4) != 0 ||
        read_exact(in, &version, 1) ||
        read_exact(in, &flags, 1) ||
        read_exact(in, &block_size_u32, 4) ||
        read_exact(in, &num_blocks_u32, 4)) {
        fprintf(stderr, "compressor: bad header in %s\n", input_path);
        fclose(in);
        return -1;
    }
    if (version != FORMAT_VERSION) {
        fprintf(stderr, "compressor: unsupported version %u\n", version);
        fclose(in);
        return -1;
    }

    bool use_rle1     = flags & 0x01;
    bool use_mtf      = flags & 0x02;
    bool use_rle2     = flags & 0x04;
    bool use_huffman  = flags & 0x08;
    bool use_bwt_meta = flags & 0x10;

    size_t cap = work_buffer_size(block_size_u32);
    unsigned char *buf_a = malloc(cap);
    unsigned char *buf_b = malloc(cap);
    if (!buf_a || !buf_b) {
        fprintf(stderr, "compressor: malloc failed\n");
        free(buf_a); free(buf_b);
        fclose(in);
        return -1;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("compressor: fopen output");
        free(buf_a); free(buf_b);
        fclose(in);
        return -1;
    }

    unsigned char mtf_arr[260];

    for (uint32_t i = 0; i < num_blocks_u32; i++) {
        uint32_t orig_size, primary_index, mtf_char_count, payload_size;
        if (read_exact(in, &orig_size, 4) ||
            read_exact(in, &primary_index, 4) ||
            read_exact(in, &mtf_char_count, 4)) {
            fprintf(stderr, "compressor: truncated block %u header\n", i);
            goto fail;
        }
        if (mtf_char_count > sizeof(mtf_arr)) {
            fprintf(stderr, "compressor: implausible mtf_char_count=%u\n",
                    mtf_char_count);
            goto fail;
        }
        if (mtf_char_count > 0 &&
            read_exact(in, mtf_arr, mtf_char_count)) {
            fprintf(stderr, "compressor: truncated mtf table\n");
            goto fail;
        }
        if (read_exact(in, &payload_size, 4)) {
            fprintf(stderr, "compressor: truncated payload_size\n");
            goto fail;
        }

        unsigned char *cur = buf_a;
        unsigned char *nxt = buf_b;
        if (read_exact(in, cur, payload_size)) {
            fprintf(stderr, "compressor: truncated payload\n");
            goto fail;
        }
        size_t len = payload_size;

        if (use_huffman) {
            size_t out_len = 0;
            huffman_decode(cur, len, nxt, &out_len);
            swap_ptr(&cur, &nxt);
            len = out_len;
        }
        if (use_rle2) {
            size_t out_len = 0;
            rle2_decode(cur, len, nxt, &out_len);
            swap_ptr(&cur, &nxt);
            len = out_len;
        }
        if (use_mtf) {
            mtf_decode(cur, len, mtf_arr, mtf_char_count, nxt);
            swap_ptr(&cur, &nxt);
            // length unchanged
        }

        // BWT decode: drop trailing sentinel byte
        if (len == 0) {
            fprintf(stderr, "compressor: block %u zero-length pre-BWT\n", i);
            goto fail;
        }
        size_t bwt_len = len - 1;
        if (use_bwt_meta) {
            bwt_decode_meta(cur, bwt_len, primary_index, nxt);
        } else {
            bwt_decode(cur, bwt_len, primary_index, nxt);
        }
        swap_ptr(&cur, &nxt);
        len = bwt_len;

        if (use_rle1) {
            // rle1_decode scans for '\0' terminator
            cur[len] = '\0';
            size_t out_len = 0;
            rle1_decode(cur, len, nxt, &out_len);
            swap_ptr(&cur, &nxt);
            len = orig_size;
        }

        if (write_exact(out, cur, orig_size)) {
            perror("compressor: fwrite");
            goto fail;
        }
    }

    free(buf_a);
    free(buf_b);
    fclose(in);
    fclose(out);
    return 0;

fail:
    free(buf_a);
    free(buf_b);
    fclose(in);
    fclose(out);
    return -1;
}
