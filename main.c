// TODO: consider putting scope start curly brace on new line instead of putting so many empty lines for better readability

#include "third-party/inih/ini.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

typedef struct
{
    static_assert(
        sizeof(char*) >= sizeof(unsigned int) 
            &&
        UINT_MAX >= 900 * 1024,
        "assumptions for optimum alignment and/or different allowed values of config.ini violated"
    );
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
#define TRY_PARSE_VALUE_AS_BOOL(param)                                  \
        if (0 == strcmp("true", value))         { param = true; }       \
        else if (0 == strcmp("false", value))   { param = false; }      \
        else                                    { return 0; }           \

static int general_handler(void* user, const char* name, const char* value) {

    configuration* pconfig = (configuration*)user;

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

        return 0;  /* unknown name, error */

    }

    return 1;
}

static int performance_handler(void* user, const char* name, const char* value) {

    configuration* pconfig = (configuration*)user;

    if (MATCH("benchmark_mode", name)) {

        TRY_PARSE_VALUE_AS_BOOL(pconfig->benchmark_mode)

    } else if (MATCH("output_metrics", name)) {

        TRY_PARSE_VALUE_AS_BOOL(pconfig->output_metrics)

    } else {

        return 0;  /* unknown name, error */

    }

    return 1;
}

static int paths_handler(void* user, const char* name, const char* value) {

    configuration* pconfig = (configuration*)user;

    if (MATCH("input_directory", name)) {

        pconfig->input_directory = strdup(value); 
        // TODO: remove all strdup and also remove heap allocations done by inih if any!!!! I want the only allowed heap allocations to be constant memory allocation at start of program!!!! To achieve this, ask sir to switch .ini files with header file, and to allow C++ so that I can do template metaprogramming (macros are painful to work with!)

    } else if (MATCH("output_directory", name)) {

        pconfig->output_directory = strdup(value);

    } else {

        return 0;  /* unknown name, error */

    }

    return 1;
}

static int handler(void* user, const char* section, const char* name, const char* value) {

    // TODO: Sir ji, please let me use variadic templates instead of C macros - I need modern C++!!!
 
    #define RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(handler_func, ...)             \
        if (0 == handler_func(__VA_ARGS__)) goto return_err_from_handler;           \


    configuration* pconfig = (configuration*)user;

    if (MATCH(" General ", section)) {

        RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(general_handler, pconfig, name, value)

    } else if (MATCH(" Performance ", section)) {

        RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(performance_handler, pconfig, name, value)

    } else if (MATCH(" Paths ", section)) {

        RUN_SPECIFIC_HANDLER_AND_RETURN_IF_ERROR(paths_handler, pconfig, name, value)

    } else {

        return_err_from_handler:

            printf("unknown section/name, error");
            return 0;  /* unknown section/name, error */

    }
    return 1;
}

int main(int argc, char* argv[])
{

    configuration config;

    if (ini_parse("config.ini", handler, &config) < 0) {
        printf("Can't load 'config.ini'\n");
        return 1;
    }

    printf("Config loaded from 'config.ini':\n");
    printf(

        "\n[ General ]\n\n\t"

        "block_size: \"%u\"\n\t"
        "rle1_enabled: \"%d\"\n\t"
        "bwt_type: \"%s\"\n\t"
        "mtf_enabled: \"%d\"\n\t"
        "rle2_enabled: \"%d\"\n\t"
        "huffman_enabled: \"%d\"\n"

        "\n[ Performance ]\n\n\t"

        "benchmark_mode: \"%d\"\n\t"
        "output_metrics: \"%d\"\n"

        "\n[ Paths ]\n\n\t"

        "input_directory: \"%s\"\n\t"
        "output_directory: \"%s\"\n\n",

        config.block_size,
        config.rle1_enabled,
        config.bwt_type,
        config.mtf_enabled,
        config.rle2_enabled,
        config.huffman_enabled,
        config.benchmark_mode,
        config.output_metrics,
        config.input_directory,
        config.output_directory
    );

    return 0;
}

