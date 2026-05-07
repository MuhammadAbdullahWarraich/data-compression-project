#include <stddef.h>
#include <stdio.h>
// TODO: Implement RLE in-place. You would think we can't encode in-place so easily. eg. abcd -> a1b1c1d1 not only is larger buffer required for encoded value, but also count of a will overwrite b.
// But, you can use a queue to handle such cases and therefore successfully encode `in-place` (queue can get really large(comparable to size of buffer) if runs are small, but we'll never know whether in-place is worse if we don't run some benchmarks)
static size_t push_number_into_cstr(unsigned char *cstr, size_t num) {
    // assumption: cstr has enough capacity
    // assumption: num != 0
    size_t n = 0;
    while (num > 0) {
        // add digits in reverse order
        unsigned char digit = num % 10;
        num /= 10;
        cstr[n++] = digit + '0';// one side effect per variable per statement is not UB; don't worry
    }
    for (size_t i = 0; i < n/2; i++) {
        // reverse digits
        unsigned char tmp = cstr[i];
        cstr[i] = cstr[n-1-i];
        cstr[n-1-i] = tmp;
    }
    return n;
}

#define WRITE_TO_OUTPUT(sep)                            \
        output[j++] = run_starter;                      \
        j += push_number_into_cstr(output+j, count);    \
        output[j++] = sep;                              \

void rle1_encode(const unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    unsigned char run_starter = *input;
    size_t count = 0;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == run_starter) { count += 1; continue; }
        WRITE_TO_OUTPUT(',')
        run_starter = input[i];
        count = 1;
    }
    WRITE_TO_OUTPUT('\0')
    *out_len = j-1;
}

size_t get_run_length(const char *input, size_t *offset) {
    size_t i = *offset;
    size_t ans = 0;
    size_t exp = 1;
    for (; input[i] <= '9' && input[i] >= '0'; i++) {
        ans *= exp;
        exp *= 10;
        ans += (size_t)(input[i] - '0');
    }
    *offset = i;
    return ans;
}

void rle1_decode(const unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;
    while (true) {
        unsigned char c = input[i++];
        size_t run_length = get_run_length(input, &i);

        for (size_t x = 0; x < run_length; x++) {
            output[j++] = c;
        }

        char comma_or_null = input[i];
        if ('\0' == comma_or_null) return;
        i += 1;
    }
    output[j] = '\0';
    *out_len = j;
}

