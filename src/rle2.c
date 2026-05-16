#include "rle2.h"
#include "bwt.h"
#include <stddef.h>
#include <stdio.h>

// This RLE is based on RLE0. This will allow efficient encoding of
// the input most MTF.

/*
 Encodes MTF output using specialized RLE
 @param input : MTF output array
 @param len : Length of input
 @param output : Output buffer
 @param out_len : Pointer to store output length
*/
void rle2_encode(unsigned char *input, size_t len, unsigned char *output,
                 size_t *out_len) {

  size_t count = 0;
  size_t idx = 0;
  for (size_t i = 0; i < len; i++) {
    if (input[i] == SENTINEL) {
      count++;
    } else {
      while (count > 0) {
        unsigned char chunk = count > 255 ? 255 : (unsigned char)count;
        output[idx++] = SENTINEL;
        output[idx++] = chunk;
        count -= chunk;
      }
      output[idx++] = input[i];
    }
  }

  // This is needed if input ends on 0s
  while (count > 0) {
    unsigned char chunk = count > 255 ? 255 : (unsigned char)count;
    output[idx++] = SENTINEL;
    output[idx++] = chunk;
    count -= chunk;
  }

  *out_len = idx;
}

/*
Decodes RLE -2 encoded data
@param input : RLE -2 encoded data
@param len : Length of encoded data
@param output : Output buffer for MTF data
@param out_len : Pointer to store output length
*/
void rle2_decode(unsigned char *input, size_t len, unsigned char *output,
                 size_t *out_len) {
  size_t idx = 0;
  for (size_t i = 0; i < len; i++) {
    if (input[i] == SENTINEL) {
      if (i + 1 >= len) {
        fprintf(stderr,
                "rle2_decode: truncated input — SENTINEL at offset %zu has no "
                "count byte\n",
                i);
        *out_len = idx;
        return;
      }
      unsigned char count = input[++i]; // next byte is the run length
      for (unsigned char j = 0; j < count; j++) {
        output[idx++] = SENTINEL;
      }
    } else {
      output[idx++] = input[i];
    }
  }
  *out_len = idx;
}