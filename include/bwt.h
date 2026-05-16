#ifndef BWT_H
#define BWT_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define uc unsigned char
#define SENTINEL 0x00

typedef struct {
  uc *rotation; // Rotation string
  size_t index; // Original index

} Rotation;

/*
Rotate an input string clockwise by 1
@param rots : an array of Rotations declared and initialised before being sent
to this function
@param input: the original input string which is to be rotated
@param len  : the length of the input
*/
uc *rotate(uc *input, size_t len);

/*
Populate Rotation table with all possible rotations and sort them
@param rots : an array of Rotations declared and initialised before being sent
to this function
@param input: the original input string which is to be rotated
@param len  : the length of the input
*/
void get_rotations(Rotation *rots, uc *input, const size_t len);

/*
Compares two rotations for sorting
@param a : First rotation
@param b : Second rotation
@return  : Comparison result ( -1 , 0 , 1)
*/
int compare_rotations(const void *a, const void *b);

/*
Sorts the given Rotation array lexicographically using qsort.
@param rots : Rotation array
@param input: the original input string which is to be rotated
@param len  : the length of the input
*/
__attribute__((deprecated(
    "qsort + strcmp on rotations is O(N^2 log N) worst case. The new "
    "prefix-doubling SA inside bwt_encode_meta replaces this; sort_rotations "
    "is kept only because the deprecated matrix-BWT path still uses it."))) void
sort_rotations(Rotation *rots, uc *input, size_t len);

/*
Extracts the last column of the sorted rotation matrix to form the BWT encoded
string
@param rots : Sorted array of Rotations
@param len  : Length of the input string
@return     : The BWT encoded string (last column)
*/
uc *encode(Rotation *rots, size_t len);

/*
Builds the first column of the BWT matrix by sorting the characters of the last
column
@param last_col : The BWT encoded string (last column)
@param len      : Length of the string
@return         : The sorted first column string
*/
uc *build_first_column(const uc *last_col, size_t len);

/*
Finds the row index of the original string in the sorted rotation matrix
@param rots  : Sorted array of Rotations
@param input : The original input string
@param len   : Length of the input string
@return      : The primary index (row index of the original string)
*/
size_t find_primary_index(Rotation *rots, uc *input, size_t len);

/*
Constructs the Last-to-First (LF) mapping array used for BWT decoding
@param first_col : The sorted first column of the BWT matrix
@param last_col  : The BWT encoded string (last column)
@param len       : Length of the string
@return          : Integer array representing the LF mapping
*/
size_t *build_ltf_mapping(const uc *first_col, const uc *last_col, size_t len);

/*
Reconstructs the original string by following the Last-to-First (LF) mapping
@param first_col     : The sorted first column of the BWT matrix
@param ltf           : The computed LF mapping array
@param primary_index : The index of the original string in the sorted matrix
@param len           : Length of the string
@param output        : Buffer to store the reconstructed original string
*/
void follow_ltf_mapping(const uc *first_col, const size_t *ltf,
                        size_t primary_index, size_t len, uc *output);

/*
Frees the memory allocated for the rotations array
@param rots : Array of Rotations to be freed
@param len  : Number of rotations in the array
*/
void free_rotations(Rotation *rots, size_t len);

/*
Forward BWT transform
@param input         : Input byte array
@param len           : Length of input
@param output        : Output buffer for BWT result
@param primary_index : Pointer to store primary index
*/
__attribute__((deprecated(
    "Legacy O(N^2) encode is deprecated. Use encode_meta instead."))) void
bwt_encode(uc *input, size_t len, uc *output, size_t *primary_index);

/*
Efficient BWT implementation using suffix arrays
@param input         : Input byte array
@param len           : Length of input
@param output        : Output buffer for BWT result
@param primary_index : Pointer to store primary index
*/
void bwt_encode_meta(uc *input, size_t len, uc *output, size_t *primary_index);

/*
 * Inverse BWT transform.
 * @param input         : BWT encoded data (last column)
 * @param len           : Length of encoded data
 * @param primary_index : Primary index from encoding
 * @param output        : Output buffer for original data (must be len+1
 * bytes)
 */
__attribute__((deprecated("Pairs with the deprecated O(N^2) bwt_encode. Use "
                          "bwt_decode_meta instead."))) void
bwt_decode(uc *input, size_t len, size_t primary_index, uc *output);

/*
 * Inverse of bwt_encode_meta. The encoded stream is `len` bytes (the sentinel
 * was elided) and primary_index records where it sat. This routine reinserts
 * the sentinel internally and returns the original `len` bytes in `output`.
 */
void bwt_decode_meta(uc *input, size_t len, size_t primary_index, uc *output);

#endif // BWT_H