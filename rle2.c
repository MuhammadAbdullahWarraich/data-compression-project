#include "include/bwt.h"
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

  int count = 0;
  int idx = 0;
  for (size_t i = 0; i < len; i++) {
    if (input[i] == SENTINEL) {
      count++;
    } else {
      if (count > 0) {
        output[idx] = SENTINEL;
        output[idx + 1] = (unsigned char)count;
        idx += 2;
        count = 0;
      }
      output[idx++] = input[i];
    }
  }

  // This is needed if input ends on 0s
  if (count > 0) {
    output[idx] = SENTINEL;
    output[idx + 1] = (unsigned char)count;
    idx += 2;
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