#include "block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Reads input file and divides into blocks
 * @param filename : Input file path
 * @param block_size : Size of each block in bytes
 * @return : BlockManager structure containing all blocks
 */
BlockManager *divide_into_blocks(const char *filename, size_t block_size)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { 
        perror("fopen"); 
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    rewind(f);

    int num_blocks = (int)(( file_size + block_size - 1) / block_size);

    BlockManager *manager = malloc(sizeof(BlockManager));
    if (!manager) { 
        fclose(f); 
        return NULL; 
    }

    manager->blocks     = malloc(num_blocks * sizeof(Block));
    manager->num_blocks = num_blocks;
    manager->block_size = block_size;


    for (int i = 0; i < num_blocks; i++)
    {
        size_t this_block_size;
        if (i == num_blocks - 1)
            this_block_size = file_size - (size_t)i * block_size;
        else
            this_block_size = block_size;

        manager->blocks[i].data = malloc(this_block_size);
        if (!manager->blocks[i].data) {
            for (int j = 0; j < i; j++) {
                free(manager->blocks[j].data);
            }
            free(manager->blocks);
            free(manager);
            fclose(f);
            return NULL;
        }

        size_t bytes_read = fread(manager->blocks[i].data, 1, this_block_size, f);
        manager->blocks[i].size          = bytes_read;
        manager->blocks[i].original_size = bytes_read;
        manager->blocks[i].primary_index = 0;
    }

    fclose(f);
    return manager;
}


/*
 * Reassembles blocks back into original file
 * @param manager : BlockManager containing processed blocks
 * @param output_filename : Path for output file
 * @return : 0 on success , -1 on failure
 */
int reassemble_blocks(BlockManager *manager, const char *output_filename)
{
    FILE *f = fopen(output_filename, "wb");
    if (!f) { 
        perror("fopen"); 
        return -1;
    }

    for (int i = 0; i < manager->num_blocks; i++)
    {
        size_t written = fwrite(manager->blocks[i].data, 1, manager->blocks[i].size, f);
        if (written != manager->blocks[i].size) {
            perror("fwrite");
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}


/*
 * Frees memory allocated for BlockManager
 * @param manager : Pointer to BlockManager to free
 */
void free_block_manager(BlockManager *manager)
{
    if (!manager) 
        return;
    for (int i = 0; i < manager->num_blocks; i++)
        free(manager->blocks[i].data);
    free(manager->blocks);
    free(manager);
}