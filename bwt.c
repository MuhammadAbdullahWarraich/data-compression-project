#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>


#define uc unsigned char

typedef struct {
uc *rotation ; // Rotation string
int index ; // Original index

}Rotation;


/*
Rotate an input string clockwise by 1
@param rots : an array of Rotations declared and initialised before being sent to this function
@param input: the original input string which is to be rotated
@param len  : the length of the input 
*/

uc *rotate(uc* input, int len)
{
    if (len <= 1) return NULL;

    uc x = input[0];
    for(size_t j = 0; j < len - 1; j++)
    {
        input[j] = input[j+1];
    }
    input[len - 1] = x;

    return input;
}

/*
Populate Rotation table with all possible rotations and sort them
@param rots : an array of Rotations declared and initialised before being sent to this function
@param input: the original input string which is to be rotated
@param len  : the length of the input 
*/

void get_rotations(Rotation *rots, uc *input, const size_t len)
{
    // size_t len = sizeof(input) / sizeof(char);
    // Rotation *rots = malloc(len * sizeof(Rotation));

    uc *copy = (uc *)malloc(len + 1);
    memcpy(copy, input, len);
    copy[len] = '\0';

    for(size_t i = 0; i < len; i++)
    {
        rotate(copy, len);
        rots[i].rotation = malloc(len + 1);
        memcpy(rots[i].rotation, copy, len + 1);
        rots[i].index = (int)i;
    }
    free(copy);
}


/*
Compares two rotations for sorting
@param a : First rotation
@param b : Second rotation
@return  : Comparison result ( -1 , 0 , 1)
*/

int compare_rotations(const void *a, const void *b)
{
    return strcmp(((const Rotation *)a)->rotation,((const Rotation *)b)->rotation);
}
/*
Sorts the given Rotation array lexicographically using qsort 
@param rots : Rotation array
@param input: the original input string which is to be rotated
@param len  : the length of the input 

*/
void sort_rotations(Rotation *rots, uc *input, size_t len)
{
    qsort(rots, len, sizeof(Rotation), compare_rotations);
}

uc* encode(Rotation *rots, size_t len)
{
    unsigned char *final = malloc(len + 1);
    for (size_t i = 0; i < len; i++)
        final[i] = (unsigned char)rots[i].rotation[len - 1];
    final[len] = '\0';

    return final;
}

uc *build_first_column(const uc *last_col, size_t len)
{
    uc *first = malloc(len);
    memcpy(first, last_col, len);

    for (size_t i = 0; i < len - 1; i++)
        for (size_t j = i + 1; j < len; j++)
            if (first[i] > first[j]) {
                uc tmp   = first[i];
                first[i] = first[j];
                first[j] = tmp;
            }

    return first;
}

int find_primary_index(Rotation *rots, uc *input, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        if(!strcmp((const char *)input, rots[i].rotation))
        return (int) i;
    }

}

int *build_ltf_mapping(const uc *first_col, const uc *last_col, size_t len)
{
    int  *ltf  = malloc(len * sizeof(int));
    bool *used = calloc(len, sizeof(bool));

    for (size_t i = 0; i < len; i++) {
        for (size_t j = 0; j < len; j++) {
            if (!used[j] && first_col[i] == last_col[j]) {
                ltf[i]  = (int)j;
                used[j] = true;
                break;
            }
        }
    }

    free(used);
    return ltf;
}

void follow_ltf_mapping(const uc *first_col, const int *ltf,
                               int primary_index, size_t len, uc *output)
{
    int idx = primary_index;
    for (size_t i = 0; i < len; i++) {
        output[i] = first_col[idx];
        idx = ltf[idx];
    }
    output[len] = '\0';
}


void free_rotations(Rotation *rots, size_t len)
{
    for (size_t i = 0; i < len; i++)
        free(rots[i].rotation);
    free(rots);
}

/*
Forward BWT transform
@param input         : Input byte array
@param len           : Length of input
@param output        : Output buffer for BWT result
@param primary_index : Pointer to store primary index
*/
void bwt_encode ( uc *input , size_t len , uc *output , int *primary_index) 
{
    Rotation *rots = malloc(len * sizeof(Rotation));
    if (!rots) return;

    get_rotations(rots, input, len);
    sort_rotations(rots, input, len);

    unsigned char *encoded = encode(rots, len);
    memcpy(output, encoded, len + 1);
    free(encoded);

    *primary_index = find_primary_index(rots, input, len);
    free_rotations(rots, len);

}

/*
 * Inverse BWT transform.
 * @param input         : BWT encoded data (last column)
 * @param len           : Length of encoded data
 * @param primary_index : Primary index from encoding
 * @param output        : Output buffer for original data (must be len+1 bytes)
 */
void bwt_decode(uc *input, size_t len, int primary_index, uc *output)
{
    uc  *first = build_first_column(input, len);
    int *ltf   = build_ltf_mapping(first, input, len);

    follow_ltf_mapping(first, ltf, primary_index, len, output);

    free(first);
    free(ltf);
}
