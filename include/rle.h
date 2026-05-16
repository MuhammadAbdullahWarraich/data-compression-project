#ifndef RLE_H
#define RLE_H

#include <stddef.h>

void rle1_encode(const unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);
void rle1_decode(const unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);

#endif // RLE_H
