// TODO: consider putting scope start curly brace on new line instead of putting
// so many empty lines for better readability

#include "block.c"
#include "huffman.c"
#include "include/bwt.h"
#include "third-party/inih/ini.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  static_assert(sizeof(char *) >= sizeof(unsigned int) &&
                    UINT_MAX >= 900 * 1024,
                "assumptions for optimum alignment and/or different allowed "
                "values of config.ini violated");
  // TODO: replace char* with stack-based C-strings of fixed size
  char *input_directory;
  char *output_directory;
  char *bwt_type;
  unsigned int block_size;
  bool rle1_enabled;
  bool mtf_enabled;
  bool rle2_enabled;
  bool huffman_enabled;
  bool benchmark_mode;
  bool output_metrics;
} configuration;

#define MATCH(a, b) strcmp(a, b) == 0
#define TRY_PARSE_VALUE_AS_BOOL(param)                                         \
  if (0 == strcmp("true", value)) {                                            \
    param = true;                                                              \
  } else if (0 == strcmp("false", value)) {                                    \
    param = false;                                                             \
  } else {                                                                     \
    return 0;                                                                  \
  }

static int general_handler(void *user, const char *name, const char *value) {

  configuration *pconfig = (configuration *)user;

  if (MATCH("block_size", name)) {

    pconfig->block_size = atoi(value);

  } else if (MATCH("rle1_enabled", name)) {

    TRY_PARSE_VALUE_AS_BOOL(pconfig->rle1_enabled)

  } else if (MATCH("bwt_type", name)) {

    pconfig->bwt_type = strdup(value); // TODO: replace str type with enum

  } else if (MATCH("mtf_enabled", name)) {

    TRY_PARSE_VALUE_AS_BOOL(pconfig->mtf_enabled)

  } else if (MATCH("rle2_enabled", name)) {

    TRY_PARSE_VALUE_AS_BOOL(pconfig->rle2_enabled)

  } else if (MATCH("huffman_enabled", name)) {

    TRY_PARSE_VALUE_AS_BOOL(pconfig->huffman_enabled)

  } else {

    return 0; /* unknown name, error */
  }

  return 1;
}

static int performance_handler(void *user, const char *name,
                               const char *value) {

  configuration *pconfig = (configuration *)user;

  if (MATCH("benchmark_mode", name)) {

    TRY_PARSE_VALUE_AS_BOOL(pconfig->benchmark_mode)

  } else if (MATCH("output_metrics", name)) {

    TRY_PARSE_VALUE_AS_BOOL(pconfig->output_metrics)

  } else {

    return 0; /* unknown name, error */
  }

  return 1;
}

static int paths_handler(void *user, const char *name, const char *value) {

  configuration *pconfig = (configuration *)user;

  if (MATCH("input_directory", name)) {

    pconfig->input_directory = strdup(value);
    // TODO: remove all strdup and also remove heap allocations done by inih if
    // any!!!! I want the only allowed heap allocations to be constant memory
    // allocation at start of program!!!! To achieve this, ask sir to switch
    // .ini files with header file, and to allow C++ so that I can do template
    // metaprogramming (macros are painful to work with!)

  } else if (MATCH("output_directory", name)) {

    pconfig->output_directory = strdup(value);

  } else {

    return 0; /* unknown name, error */
  }

  return 1;
}

static int handler(void *user, const char *section, const char *name,
                   const char *value) {

  // TODO: Sir ji, please let me use variadic templates instead of C macros - I
  // need modern C++!!!

#define RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(handler_func, ...)            \
  if (0 == handler_func(__VA_ARGS__))                                          \
    goto return_err_from_handler;

  configuration *pconfig = (configuration *)user;

  if (MATCH(" General ", section)) {

    RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(general_handler, pconfig, name,
                                             value)

  } else if (MATCH(" Performance ", section)) {

    RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(performance_handler, pconfig, name,
                                             value)

  } else if (MATCH(" Paths ", section)) {

    RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(paths_handler, pconfig, name,
                                             value)

  } else {

  return_err_from_handler:

    printf("unknown section/name, error");
    return 0; /* unknown section/name, error */
  }
  return 1;
}

void rle1_encode(const unsigned char *input, size_t len, unsigned char *output,
                 size_t *out_len);
void rle1_decode(const unsigned char *input, size_t len, unsigned char *output,
                 size_t *out_len);

void test_rle() {
  const unsigned char *input = (const unsigned char *)"aaabccdadd";
  size_t input_size = strlen((const char *)input);
  size_t out_len = 0;

  unsigned char *output = malloc(2 * input_size);
  rle1_encode(input, input_size, output, &out_len);
  printf("encoding:\n\t"
         "input: \"%s\"\n\t"
         "output: \"%s\"\n",
         input, output);
  unsigned char *output2 = malloc(2 * input_size);
  size_t out2_len = 0;

  rle1_decode(output, out_len, output2, &out2_len);
  printf("decoding:\n\t"
         "input: \"%s\"\n\t"
         "output: \"%s\"\n",
         output, output2);
}

void test_blocks() {
  FILE *f = fopen("test_input.bin", "wb");
  fwrite("AAABBBCCCDDDEEE", 1, 15, f);
  fclose(f);

  BlockManager *mgr = divide_into_blocks("test_input.bin", 4);
  printf("num_blocks: %d\n", mgr->num_blocks);
  for (int i = 0; i < mgr->num_blocks; i++) {
    printf("block[%d]: size=%zu data=\"%.*s\"\n", i, mgr->blocks[i].size,
           (int)mgr->blocks[i].size, mgr->blocks[i].data);
  }
  reassemble_blocks(mgr, "test_output.bin");

  char reconstructed[256] = {0};
  f = fopen("test_output.bin", "rb");
  fread(reconstructed, 1, 255, f);
  fclose(f);

  if (strcmp(reconstructed, "AAABBBCCCDDDEEE") == 0)
    printf("match: yes\n");
  else
    printf("match: no\n");

  free_block_manager(mgr);
}

void test_huffman()
{
    unsigned char input[] = "aabbbcccc";
    size_t len = strlen((char *)input);

    unsigned char *compressed = malloc(264 + 2 * len);
    unsigned char *decompressed = malloc(len + 1);
    size_t comp_len = 0, decomp_len = 0;

    huffman_encode(input, len, compressed, &comp_len);
    huffman_decode(compressed, comp_len, decompressed, &decomp_len);

    printf("original:     %s\n", input);
    printf("decompressed: %s\n", decompressed);
    if (strcmp((char *)input, (char *)decompressed) == 0)
        printf("match: YES\n");
    else
        printf("match: NO\n");

    free(compressed);
    free(decompressed);
}

int main(int argc, char *argv[]) {

  configuration config;

  if (ini_parse("config.ini", handler, &config) < 0) {
    printf("Can't load 'config.ini'\n");
    return 1;
  }

  printf("Config loaded from 'config.ini':\n");
  printf(

      "\n[ General ]\n\n\t"

      "block_size: %u\n\t"
      "rle1_enabled: %d\n\t"
      "bwt_type: \"%s\"\n\t"
      "mtf_enabled: %d\n\t"
      "rle2_enabled: %d\n\t"
      "huffman_enabled: %d\n"

      "\n[ Performance ]\n\n\t"

      "benchmark_mode: %d\n\t"
      "output_metrics: %d\n"

      "\n[ Paths ]\n\n\t"

      "input_directory: \"%s\"\n\t"
      "output_directory: \"%s\"\n\n",

      config.block_size, config.rle1_enabled, config.bwt_type,
      config.mtf_enabled, config.rle2_enabled, config.huffman_enabled,
      config.benchmark_mode, config.output_metrics, config.input_directory,
      config.output_directory);

  // TEST STRING -- SAMPLE IMPLEMENTATION

  uc input[] = "the_cat_sat_on_the_mat_and_the_rat_sat_on_the_cat";
  size_t len = strlen((char *)input);

  uc *encoded = malloc(len + 1);
  uc *decoded = malloc(len + 1);
  size_t primary_index = -1;

  bwt_encode(input, len, encoded, &primary_index);
  printf("Input:         %s\n", input);
  printf("Encoded:       %s\n", encoded);
  printf("Primary index: %zu\n", primary_index);

  bwt_decode(encoded, len, primary_index, decoded);
  printf("Decoded:       %s\n", decoded);
  printf("Match:         %s\n",
         strcmp((char *)input, (char *)decoded) == 0 ? "YES" : "NO");

  encoded = malloc(len + 1);
  decoded = malloc(len + 1);
  primary_index = -1;
  bwt_encode_meta(input, len, encoded, &primary_index);
  printf("Input:         %s\n", input);
  printf("Encoded_Meta:       %s\n", encoded);
  printf("Primary index: %zu\n", primary_index);

  bwt_decode_meta(encoded, len, primary_index, decoded);
  printf("Decoded:       %s\n", decoded);
  printf("Match:         %s\n",
         strcmp((char *)input, (char *)decoded) == 0 ? "YES" : "NO");

  free(encoded);

  test_rle();

  printf("Block_Testing\n");
  test_blocks();
  printf("Huffman_Testing\n");
  test_huffman();

  return 0;
}
