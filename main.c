#include "inih/ini.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    const char* bwt_type;
    int block_size;
    bool huffman_enabled;
} configuration;

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    configuration* pconfig = (configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("General", "block_size")) {

        pconfig->block_size = atoi(value);

    } else if (MATCH("General", "bwt_type")) {

        pconfig->bwt_type = strdup(value);

    } else if (MATCH("General", "huffman_enabled")) {

        const char* true_str = "true";
        const char* false_str = "false";

        if (strncmp(true_str, value, sizeof true_str))          { pconfig->huffman_enabled = true; } 
        else if (strncmp(false_str, value, sizeof false_str))   { pconfig->huffman_enabled = false; } 
        else                                                    { return 0; }

    } else {
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
    printf("Config loaded from 'config.ini': block_size=%d, bwt_type=%s, huffman_enabled=%d\n",
        config.block_size, config.bwt_type, config.huffman_enabled);
    return 0;
}

