#include "compressor.h"
#include "third-party/inih/ini.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    static_assert(sizeof(char *) >= sizeof(unsigned int) &&
                      UINT_MAX >= 900 * 1024,
                  "assumptions for optimum alignment and/or different allowed "
                  "values of config.ini violated");
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
    if (0 == strcmp("true", value)) {                                          \
        param = true;                                                          \
    } else if (0 == strcmp("false", value)) {                                  \
        param = false;                                                         \
    } else {                                                                   \
        return 0;                                                              \
    }

static int general_handler(void *user, const char *name, const char *value) {
    configuration *pconfig = (configuration *)user;
    if (MATCH("block_size", name)) {
        pconfig->block_size = atoi(value);
    } else if (MATCH("rle1_enabled", name)) {
        TRY_PARSE_VALUE_AS_BOOL(pconfig->rle1_enabled)
    } else if (MATCH("bwt_type", name)) {
        pconfig->bwt_type = strdup(value);
    } else if (MATCH("mtf_enabled", name)) {
        TRY_PARSE_VALUE_AS_BOOL(pconfig->mtf_enabled)
    } else if (MATCH("rle2_enabled", name)) {
        TRY_PARSE_VALUE_AS_BOOL(pconfig->rle2_enabled)
    } else if (MATCH("huffman_enabled", name)) {
        TRY_PARSE_VALUE_AS_BOOL(pconfig->huffman_enabled)
    } else {
        return 0;
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
        return 0;
    }
    return 1;
}

static int paths_handler(void *user, const char *name, const char *value) {
    configuration *pconfig = (configuration *)user;
    if (MATCH("input_directory", name)) {
        pconfig->input_directory = strdup(value);
    } else if (MATCH("output_directory", name)) {
        pconfig->output_directory = strdup(value);
    } else {
        return 0;
    }
    return 1;
}

static int handler(void *user, const char *section, const char *name,
                   const char *value) {
#define RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(handler_func, ...)            \
    if (0 == handler_func(__VA_ARGS__))                                        \
        goto return_err_from_handler;

    configuration *pconfig = (configuration *)user;
    if (MATCH(" General ", section)) {
        RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(general_handler, pconfig, name,
                                                 value)
    } else if (MATCH(" Performance ", section)) {
        RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(performance_handler, pconfig,
                                                 name, value)
    } else if (MATCH(" Paths ", section)) {
        RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(paths_handler, pconfig, name,
                                                 value)
    } else {
    return_err_from_handler:
        printf("unknown section/name, error");
        return 0;
    }
    return 1;
}

static long file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long s = ftell(f);
    fclose(f);
    return s;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s compress   <input> [output]\n"
        "  %s decompress <input> [output]\n"
        "\n"
        "Compression parameters are read from config.ini in the current dir.\n"
        "Default output suffix is .bwtc (compress) / .out (decompress).\n",
        prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    const char *mode = argv[1];
    const char *input_path = argv[2];

    configuration config = {0};
    if (ini_parse("config.ini", handler, &config) < 0) {
        fprintf(stderr, "Can't load 'config.ini'\n");
        return 1;
    }

    if (strcmp(mode, "compress") == 0) {
        char default_out[1024];
        const char *output_path;
        if (argc >= 4) {
            output_path = argv[3];
        } else {
            snprintf(default_out, sizeof(default_out), "%s.bwtc", input_path);
            output_path = default_out;
        }

        CompressorConfig cc = {
            .block_size       = config.block_size,
            .rle1_enabled     = config.rle1_enabled,
            .mtf_enabled      = config.mtf_enabled,
            .rle2_enabled     = config.rle2_enabled,
            .huffman_enabled  = config.huffman_enabled,
            .bwt_suffix_array = config.bwt_type &&
                                strcmp(config.bwt_type, "suffix_array") == 0,
        };

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        int rc = compressor_compress(input_path, output_path, &cc);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (rc != 0) return 1;

        if (config.output_metrics) {
            double elapsed = (t1.tv_sec - t0.tv_sec) +
                             (t1.tv_nsec - t0.tv_nsec) / 1e9;
            long in_sz = file_size(input_path);
            long out_sz = file_size(output_path);
            double ratio = in_sz > 0 ? (double)out_sz / (double)in_sz : 0.0;
            printf("compress: %s -> %s\n", input_path, output_path);
            printf("  input  : %ld bytes\n", in_sz);
            printf("  output : %ld bytes (ratio %.4f, saved %.2f%%)\n",
                   out_sz, ratio, (1.0 - ratio) * 100.0);
            printf("  time   : %.3f s\n", elapsed);
        }
        return 0;
    } else if (strcmp(mode, "decompress") == 0) {
        char default_out[1024];
        const char *output_path;
        if (argc >= 4) {
            output_path = argv[3];
        } else {
            snprintf(default_out, sizeof(default_out), "%s.out", input_path);
            output_path = default_out;
        }

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        int rc = compressor_decompress(input_path, output_path);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (rc != 0) return 1;

        if (config.output_metrics) {
            double elapsed = (t1.tv_sec - t0.tv_sec) +
                             (t1.tv_nsec - t0.tv_nsec) / 1e9;
            printf("decompress: %s -> %s (%.3f s)\n",
                   input_path, output_path, elapsed);
        }
        return 0;
    } else {
        usage(argv[0]);
        return 1;
    }
}
