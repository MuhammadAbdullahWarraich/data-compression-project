#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
/*
* Forward MTF transform
* @param input : Input byte array
* @param len : Length of input
* @param output : Output buffer for MTF indices
*/
#define SENTINEL '\0'
void mtf_encode(unsigned char *input, size_t len, unsigned char *output, size_t *char_count, unsigned char **sorted_chars) {
    bool char_is_in_array[256];
    for (size_t i = 0; i < 256; i++) { char_is_in_array[i] = false; }

    size_t n = 0;
    for (size_t i = 0; i < len-1; i++) {
        const unsigned char c = input[i];
        assert(c != SENTINEL);
        if (!char_is_in_array[c]) {
            char_is_in_array[c] = true;
            n += 1;
        }
    }
    n += 1;// add space for sentinel

    unsigned char *arr = malloc(sizeof(size_t) * n);
    if (NULL == arr) {
        printf("malloc failed in mtf_encode! get more RAM!(only possible failure of malloc is ENOMEM, haha)\n");
        exit(1);
    }

    for (size_t i = 0, j = 0; i < 256; i++) {
        if (char_is_in_array[i]) {
            arr[j++] = i;
        }
    }
    arr[n-1] = SENTINEL;

    *sorted_chars = malloc(sizeof(size_t) * n);
    if (NULL == *sorted_chars) {
        printf("malloc failed in mtf_encode! get more RAM!(only possible failure of malloc is ENOMEM, haha)\n");
        exit(1);
    }
    for (size_t i = 0; i < n; i++) {
        (*sorted_chars)[i] = arr[i];
    }
    *char_count = n;

    for (size_t i = 0, j = 0; i < len; i++) {
        for (size_t x = 0; x < n; x++) {
            if (arr[x] == input[i]) {
                output[j++] = x;
                if (x != 0) {
                    // swapping
                    arr[x] ^= arr[0];
                    arr[0] ^= arr[x];
                    arr[x] ^= arr[0];
                }
                break;
            }
        }
    }
}

 /*
 * Inverse MTF transform
 * @param input : MTF encoded indices
 * @param len : Length of input
 * @param output : Output buffer for decoded data
 */
void mtf_decode(unsigned char *input, size_t len, unsigned char *arr, size_t char_count, unsigned char *output) {
    for (size_t i = 0; i < len; i++) {
        output[i] = arr[input[i]];
        if (input[i] != 0) {
            // swapping
            arr[input[i]]   ^= arr[0];
            arr[0]          ^= arr[input[i]];
            arr[input[i]]   ^= arr[0];
        }
    }
}

int main() {
    char input[] = {'f', 'e', 'd', 'c', 'b', 'a', SENTINEL, '\0'};
    const size_t input_size = 7;

    printf("input: { ");
    for (size_t i = 0; i < input_size; i++) {
        printf("'%d' ", input[i]);
    }
    printf(" }\n");

    char output[input_size];

    unsigned char *sorted_chars = NULL;
    size_t char_count = 0;

    mtf_encode(input, input_size, output, &char_count, &sorted_chars);
    printf("after encoding: { ");
    for (size_t i = 0; i < input_size; i++) {
        printf("'%d' ", output[i]);
    }
    printf(" }\n");
    printf("{ ");
    for (size_t i = 0; i < char_count; i++) {
        printf("'%d' ", sorted_chars[i]);
    }
    printf(" }\n");

    mtf_decode(output, input_size, sorted_chars, char_count, input);

    printf("after decoding\n{ ");
    for (size_t i = 0; i < input_size; i++) {
        printf("'%d' ", input[i]);
    }
    printf(" }\n");
}
