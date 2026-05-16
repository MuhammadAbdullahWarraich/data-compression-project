#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stddef.h>

void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len);

void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len);

#endif // HUFFMAN_H
