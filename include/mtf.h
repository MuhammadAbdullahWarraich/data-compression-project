#ifndef MTF_H
#define MTF_H

#include <stddef.h>

void mtf_encode(unsigned char *input, size_t len,
                unsigned char *output,
                size_t *char_count, unsigned char **sorted_chars);

void mtf_decode(unsigned char *input, size_t len,
                unsigned char *arr, size_t char_count,
                unsigned char *output);

#endif // MTF_H
