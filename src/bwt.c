#include "bwt.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ─── ROTATION HELPERS (used by the matrix BWT path) ──────────────────────────

uc *rotate(uc *input, size_t len) {
  if (len <= 1)
    return NULL;

  uc x = input[0];
  for (size_t j = 0; j < len - 1; j++) {
    input[j] = input[j + 1];
  }
  input[len - 1] = x;

  return input;
}

void get_rotations(Rotation *rots, uc *input, const size_t len) {
  uc *copy = (uc *)malloc(len + 1);
  memcpy(copy, input, len);
  copy[len] = '\0';

  for (size_t i = 0; i < len; i++) {
    rotate(copy, len);
    rots[i].rotation = malloc(len + 1);
    memcpy(rots[i].rotation, copy, len + 1);
    rots[i].index = i;
  }

  free(copy);
}

uc *encode(Rotation *rots, size_t len) {
  unsigned char *final = malloc(len + 1);
  for (size_t i = 0; i < len; i++)
    final[i] = (unsigned char)rots[i].rotation[len - 1];
  final[len] = '\0';

  return final;
}

uc *build_first_column(const uc *last_col, size_t len) {
  uc *first = malloc(len);
  memcpy(first, last_col, len);

  for (size_t i = 0; i < len - 1; i++)
    for (size_t j = i + 1; j < len; j++)
      if (first[i] > first[j]) {
        uc tmp = first[i];
        first[i] = first[j];
        first[j] = tmp;
      }

  return first;
}

size_t find_primary_index(Rotation *rots, uc *input, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!strcmp((const char *)input, (const char *)rots[i].rotation))
      return i;
  }
  return (size_t)-1;
}

size_t *build_ltf_mapping(const uc *first_col, const uc *last_col, size_t len) {
  size_t *ltf = malloc(len * sizeof(size_t));
  bool *used = calloc(len, sizeof(bool));

  for (size_t i = 0; i < len; i++) {
    for (size_t j = 0; j < len; j++) {
      if (!used[j] && first_col[i] == last_col[j]) {
        ltf[i] = j;
        used[j] = true;
        break;
      }
    }
  }

  free(used);
  return ltf;
}

void follow_ltf_mapping(const uc *first_col, const size_t *ltf,
                        size_t primary_index, size_t len, uc *output) {
  size_t idx = primary_index;
  for (size_t i = 0; i < len; i++) {
    output[i] = first_col[idx];
    idx = ltf[idx];
  }
  output[len] = '\0';
}

void free_rotations(Rotation *rots, size_t len) {
  for (size_t i = 0; i < len; i++)
    free(rots[i].rotation);
  free(rots);
}

// ─── ROTATION SORTING ────────────────────────────────────────────────────────
//
// Two implementations live here:
//
//   sort_rotations (DEPRECATED) — qsort + strcmp on materialised rotation
//   strings. O(N^2 log N) worst case on repetitive inputs. Kept only because
//   the matrix-BWT path (bwt_encode / bwt_decode) still references it.
//
//   sort_rotations_meta — prefix-doubling suffix array. O(N log N) overall.
//   Used by bwt_encode_meta.

int compare_rotations(const void *a, const void *b) {
  return strcmp((const char *)((const Rotation *)a)->rotation,
                (const char *)((const Rotation *)b)->rotation);
}

void sort_rotations(Rotation *rots, uc *input, size_t len) {
  qsort(rots, len, sizeof(Rotation), compare_rotations);
}

// Prefix-doubling SA, adapted from Thomas Mailund's reference implementation:
// https://mailund.dk/posts/prefix-doubling-attemps/
//
// Two-level radix sort, doubling the comparison window (k = 1, 2, 4, ...) each
// round until every suffix has a unique rank. Each round is O(N) thanks to
// bucket-sort, giving O(N log N) overall.
//
// Only deviation from the article: remap_n takes an explicit length instead
// of stopping at the first 0 byte, since our BWT input contains a real 0x00
// sentinel that must participate in the SA.

#define NO_CHARS (1 << CHAR_BIT)
#define SWAP(a, b)             \
    do                         \
    {                          \
        __typeof__(a) tmp = a; \
        a = b;                 \
        b = tmp;               \
    } while (0)

static void *faultless_malloc(size_t size)
{
    void *mem = malloc(size);
    if (!mem)
        abort();
    return mem;
}

static uint32_t *remap_n(const unsigned char *x, uint32_t n, uint32_t *sigma)
{
    uint32_t alphabet[NO_CHARS] = {0};
    for (uint32_t i = 0; i < n; i++)
        alphabet[x[i]] = 1;

    *sigma = 1;
    for (int a = 0; a < NO_CHARS; a++)
    {
        if (alphabet[a])
            alphabet[a] = (*sigma)++;
    }

    uint32_t *mapped = faultless_malloc(n * sizeof *mapped);
    for (uint32_t i = 0; i < n; i++)
        mapped[i] = alphabet[x[i]];

    return mapped;
}

static uint32_t *alloc_sa(uint32_t n)
{
    uint32_t *sa = faultless_malloc(n * sizeof *sa);
    for (uint32_t i = 0; i < n; i++)
    {
        sa[i] = i;
    }
    return sa;
}

static inline int get_rank(uint32_t n,
                           uint32_t rank[static n],
                           int i)
{
    return (i < (int)n) ? rank[i] : 0;
}

static void bsort(uint32_t n,
                  uint32_t sa[static n],
                  uint32_t rank[static n],
                  uint32_t out[static n],
                  uint32_t k,
                  uint32_t buckets[static n],
                  uint32_t sigma)
{
    memset(buckets, 0, sigma * sizeof *buckets);

    for (uint32_t i = 0; i < n; i++)
    {
        buckets[get_rank(n, rank, sa[i] + k)]++;
    }
    for (uint32_t acc = 0, i = 0; i < sigma; i++)
    {
        uint32_t b = buckets[i];
        buckets[i] = acc;
        acc += b;
    }
    for (uint32_t i = 0; i < n; i++)
    {
        uint32_t j = sa[i];
        uint32_t r = get_rank(n, rank, j + k);
        out[buckets[r]++] = j;
    }
}

static uint32_t update_rank(uint32_t n,
                            uint32_t sa[static n],
                            uint32_t rank[static n],
                            uint32_t out[static n],
                            uint32_t k)
{
#define PAIR(i, k) (((uint64_t)rank[sa[i]] << 32) | \
                    (uint64_t)get_rank(n, rank, sa[i] + k))

    uint32_t sigma = 1;
    out[sa[0]] = sigma;

    uint64_t prev_pair = PAIR(0, k);
    for (uint32_t i = 1; i < n; i++)
    {
        uint64_t cur_pair = PAIR(i, k);
        sigma += (prev_pair != cur_pair);
        prev_pair = cur_pair;
        out[sa[i]] = sigma;
    }

    return sigma + 1;

#undef PAIR
}

// Entry point for the prefix-doubling sort. Drives the loop and owns the
// scratch buffers; per-round work is delegated to the static helpers above.
static uint32_t *sort_rotations_meta(const unsigned char *x, uint32_t n)
{
    uint32_t sigma;
    uint32_t *rank    = remap_n(x, n, &sigma);
    uint32_t *sa      = alloc_sa(n);
    uint32_t *buckets = faultless_malloc(n * sizeof *buckets);
    uint32_t *buf     = faultless_malloc(n * sizeof *buf);

    for (uint32_t k = 1; sigma < n + 1; k *= 2)
    {
        bsort(n, sa, rank, buf, k, buckets, sigma);
        bsort(n, buf, rank, sa, 0, buckets, sigma);
        sigma = update_rank(n, sa, rank, buf, k);
        SWAP(rank, buf);
    }

    free(rank);
    free(buckets);
    free(buf);

    return sa;
}

// ─── BWT TRANSFORMS ──────────────────────────────────────────────────────────

void bwt_encode(uc *input, size_t len, uc *output, size_t *primary_index) {

  uc *input_sen = malloc(len + 1);
  memcpy(input_sen, input, len);
  input_sen[len] = SENTINEL;
  size_t slen = len + 1;

  Rotation *rots = malloc(len * sizeof(Rotation));
  if (!rots)
    return;

  get_rotations(rots, input_sen, len);
  sort_rotations(rots, input_sen, len);

  unsigned char *encoded = encode(rots, len);
  memcpy(output, encoded, len + 1);
  free(encoded);

  *primary_index = find_primary_index(rots, input_sen, len);
  free_rotations(rots, len);
}

void bwt_encode_meta(uc *input, size_t len, uc *output, size_t *primary_index) {
  uc *input_sen = malloc(len + 2);
  memcpy(input_sen, input, len);
  input_sen[len] = SENTINEL;
  input_sen[len + 1] = '\0';
  size_t slen = len + 1;

  uint32_t *sa = sort_rotations_meta(input_sen, (uint32_t)slen);

  // Walk the sorted SA producing BWT chars, but skip the row whose BWT
  // char would be the sentinel. Its position is recorded in primary_index
  // so the decoder can reinsert the sentinel before inverting.
  size_t out_i = 0;
  for (size_t i = 0; i < slen; i++) {
    uint32_t idx = sa[i];
    if (idx == 0) {
      *primary_index = i;
    } else {
      output[out_i++] = input_sen[idx - 1];
    }
  }
  output[len] = '\0';

  free(sa);
  free(input_sen);
}

void bwt_decode(uc *input, size_t len, size_t primary_index, uc *output) {
  uc *first = build_first_column(input, len);
  size_t *ltf = build_ltf_mapping(first, input, len);

  follow_ltf_mapping(first, ltf, primary_index, len, output);

  free(first);
  free(ltf);
}

void bwt_decode_meta(uc *input, size_t len, size_t primary_index, uc *output) {
  size_t slen = len + 1;

  // Reinsert the sentinel byte at primary_index to recover the slen-BWT
  // of (input + sentinel).
  uc *full = malloc(slen);
  if (!full)
    return;
  memcpy(full, input, primary_index);
  full[primary_index] = SENTINEL;
  memcpy(full + primary_index + 1, input + primary_index, len - primary_index);

  uc *first = build_first_column(full, slen);
  size_t *ltf = build_ltf_mapping(first, full, slen);

  // Walk slen rows but only emit the first len chars; the slen-th would be
  // the trailing sentinel.
  size_t idx = primary_index;
  for (size_t i = 0; i < len; i++) {
    output[i] = first[idx];
    idx = ltf[idx];
  }
  output[len] = '\0';

  free(first);
  free(ltf);
  free(full);
}
