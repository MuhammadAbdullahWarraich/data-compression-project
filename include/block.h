#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>

/*
 * A single block of data flowing through the pipeline.
 */
typedef struct {
    unsigned char *data;        // Pointer to block data
    size_t size;                // Current size of block
    size_t original_size;       // Original size before compression
    int primary_index;          // BWT primary index for this block
} Block;

/*
 * Container for all blocks of an input file.
 */
typedef struct {
    Block *blocks;
    int num_blocks;
    size_t block_size;
} BlockManager;

BlockManager *divide_into_blocks(const char *filename, size_t block_size);
int reassemble_blocks(BlockManager *manager, const char *output_filename);
void free_block_manager(BlockManager *manager);

#endif // BLOCK_H
