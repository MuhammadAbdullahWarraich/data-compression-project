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
  return strcmp(((const Rotation *)a)->rotation,
                ((const Rotation *)b)->rotation);
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
    if (!strcmp((const char *)input, rots[i].rotation))
      return i;
  }
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
  input_sen[len] = 0x00;
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

static uc *encode_meta(Rotation *rots, uc *input, size_t len) {
  unsigned char *final_meta = malloc(len + 1);

  for (size_t i = 0; i < len; i++) {
    if (rots[i].index == 0) {
      final_meta[i] = input[len - 1];
    } else {
      final_meta[i] = input[rots[i].index - 1];
    }
  }

  final_meta[len] = '\0';
  return final_meta;
}

size_t find_primary_index_meta(Rotation *rots, unsigned char *input,
                               size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (rots[i].index == 0) {
      return i;
    }
  }
}

void bwt_encode_meta(unsigned char *input, size_t len, unsigned char *output,
                     size_t *primary_index) {
  uc *input_sen = malloc(len + 2);
  memcpy(input_sen, input, len);
  input_sen[len] = 0x00;
  input_sen[len + 1] = '\0';
  size_t slen = len + 1;

  Rotation *rots = malloc(slen * sizeof(Rotation));
  if (!rots)
    return;

  get_suffixes(rots, input_sen, slen);
  sort_rotations(rots, input_sen, slen);

  uc *encoded = encode_meta(rots, input_sen, slen);
  size_t sentinel_row = find_primary_index_meta(rots, input_sen, slen);

  size_t out_i = 0;
  for (size_t i = 0; i < slen; i++) {
    if (i == sentinel_row)
      continue;
    output[out_i++] = encoded[i];
  }
  output[len] = '\0';

  *primary_index = find_primary_index_meta(rots, input_sen, len);

  free(encoded);
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