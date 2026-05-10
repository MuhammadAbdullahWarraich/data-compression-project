#include "./include/bwt.h"
#include <stddef.h>
#include <string.h>

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
  // size_t len = sizeof(input) / sizeof(char);
  // Rotation *rots = malloc(len * sizeof(Rotation));

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

int compare_rotations(const void *a, const void *b) {
  return strcmp((const char *)((const Rotation *)a)->rotation,
                (const char *)((const Rotation *)b)->rotation);
}

void sort_rotations(Rotation *rots, uc *input, size_t len) {
  qsort(rots, len, sizeof(Rotation), compare_rotations);
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

static void get_suffixes(Rotation *rots, uc *input, const size_t len) {
  for (size_t i = 0; i < len; i++) {
    rots[i].rotation = (uc *)input + i;
    rots[i].index = i;
  }
}

void bwt_encode_meta(uc *input, size_t len, uc *output, size_t *primary_index) {
  uc *input_sen = malloc(len + 2);
  memcpy(input_sen, input, len);
  input_sen[len] = SENTINEL;
  input_sen[len + 1] = '\0';
  size_t slen = len + 1;

  Rotation *rots = malloc(slen * sizeof(Rotation));
  if (!rots)
    return;

  get_suffixes(rots, input_sen, slen);
  sort_rotations(rots, input_sen, slen);

  // Walk the sorted SA producing BWT chars, but skip the row whose BWT
  // char would be the sentinel. Its position is recorded in primary_index
  // so the decoder can reinsert the sentinel before inverting.
  size_t out_i = 0;
  for (size_t i = 0; i < slen; i++) {
    if (rots[i].index == 0) {
      *primary_index = i;
    } else {
      output[out_i++] = input_sen[rots[i].index - 1];
    }
  }
  output[len] = '\0';

  free(rots);
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