#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <assert.h>

#define BACK_SCAN_LIMIT 4 * 1024

#define min(a,b) ((a) < (b) ? (a) : (b))

#define setup_for_cleanup()     \
        int ret_val = 0;        \

#define return_cleanly(rv)      \
        ret_val = rv;           \
        goto cleanup;           \

void construct_lps(char* pat, size_t* lps, size_t sz) {
    lps[0] = 0;
    size_t len = 0;
    for (size_t i = 1; i < sz; i++) {
        if (pat[len] == pat[i]) {
            len += 1;
            lps[i] = len;
        } else if (len != 0) {
            len = lps[len-1];
        } else {
            lps[i] = 0;
        }
    }
}

typedef struct Match {
    size_t st;
    size_t cnt;
};

// TODO: make threshold a macro in some common config header file to allow inlining and hence save stack space
// TODO: make lps array global to prevent repeated heap allocations and deallocations; maybe we should test both versions for time to see which is faster

int kmp(char* input, size_t input_sz, size_t la_st, size_t threshold, Match* match) {

    // RETURN VALUE: returns 0 if no match found, and 1 if match found. On failure, optionally errno is set and -1 is returned
    
    assert(threshold > 0);

    setup_for_cleanup();

    // if la_st < BACK_SCAN_LIMIT, then we only have la_st elements in the search buffer,
    // otherwise we have BACK_SCAN_LIMIT elements
    const size_t sb_sz = min(BACK_SCAN_LIMIT, la_st);
    const size_t la_sz = min(BACK_SCAN_LIMIT, input_sz - la_st);
    if (la_sz < threshold || sb_sz == 0) { return 0; }

    size_t sb_st = la_st - sb_sz;

    // The  malloc()  and calloc() functions return a pointer to the allocated memory,
    // which is suitably aligned for any built-in type.
    //                   ^^^^^^^ so we needn't worry about any element being split across cache lines.
    // Therefore, there is no need of aligned_alloc here - malloc will do just fine,
    // provided the following condition holds:
    static_assert(sizeof(size_t) <= alignof(max_align_t));
    // in cases the above assertion is false, replace the malloc below with aligned_alloc
    // Question: Why do we care about alignment in single-threaded, non-SIMD code?
    // Answer: Because in most architectures, non-aligned memory accesses are slower because of things 
    // like individual elements being split across cache lines
    // https://www.reddit.com/r/C_Programming/comments/na4l6v/how_big_a_deal_is_memory_alignment_these_days_on/
    // https://en.cppreference.com/w/c/types/size_t.html

    // we are using size_t for element type because in case of prefix match of sb_sz elements,
    // the last element of the array will have the value (sb_sz-1) which may overflow if we use
    // int because sb_sz has type size_t (size_t >= int)
    // TODO: replace size_t with unsigned short because BACK_SCAN_LIMIT is only 8192 bytes
    size_t* lps = (size_t *) malloc(la_sz * sizeof(size_t));
    if (lps == NULL) return_cleanly(-1);

    construct_lps(input+la_st, lps, la_sz);

cleanup:
    free(lps);
    return ret_val;
}
