#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Huffman code representation
 */
typedef struct {
    unsigned short code;        // Huffman code
    unsigned char length;       // Code length in bits
} HuffmanCode;

/*
 * Huffman tree node
 */
typedef struct Node {
    unsigned char symbol;       // Byte value (0-255)
    int freq;                   // Frequency count
    struct Node *left;          // Left child
    struct Node *right;         // Right child
} HuffmanNode;


static HuffmanNode *pop_min(HuffmanNode **pool, int *size)
{
    int min_i = 0;
    for (int i = 1; i < *size; i++)
        if (pool[i]->freq < pool[min_i]->freq) min_i = i;

    HuffmanNode *min = pool[min_i];
    pool[min_i]      = pool[--(*size)];
    return min;
}


static HuffmanNode *create_node(unsigned char symbol, int freq)
{
    HuffmanNode *n  = malloc(sizeof(HuffmanNode));
    n->symbol       = symbol;
    n->freq         = freq;
    n->left         = NULL;
    n->right        = NULL;
    return n;
}


// ─── TREE ────────────────────────────────────────────────────────────────────

/*
 * Builds Huffman tree from frequency counts
 * @param frequencies : Array of 256 frequency counts
 * @param root : Pointer to store root of Huffman tree
 */
void build_huffman_tree(int *frequencies, HuffmanNode **root)
{
    HuffmanNode *pool[512];
    int size = 0;

    for (int i = 0; i < 256; i++)
        if (frequencies[i] > 0)
            pool[size++] = create_node((unsigned char)i, frequencies[i]);

    while (size > 1)
    {
        HuffmanNode *left   = pop_min(pool, &size);
        HuffmanNode *right  = pop_min(pool, &size);
        HuffmanNode *parent = create_node(0, left->freq + right->freq);
        parent->left  = left;
        parent->right = right;
        pool[size++]  = parent;
    }

    *root = pool[0];
}
static void free_tree(HuffmanNode *node)
{
    if (!node) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}


// ─── CANONICAL CODES ─────────────────────────────────────────────────────────

// helper: walk tree to collect each symbol's code length
static void collect_lengths(HuffmanNode *node, unsigned char *lengths, int depth)
{
    if (!node) return;

    if (!node->left && !node->right)    // leaf
    {
        lengths[node->symbol] = (unsigned char)depth;
        return;
    }

    collect_lengths(node->left,  lengths, depth + 1);
    collect_lengths(node->right, lengths, depth + 1);
}

// helper: sort symbols by (length, symbol) for canonical assignment
typedef struct {
    unsigned char symbol;
    unsigned char length;
} SymbolLength;

static int cmp_symbol_length(const void *a, const void *b)
{
    const SymbolLength *x = (const SymbolLength *)a;
    const SymbolLength *y = (const SymbolLength *)b;
    if (x->length != y->length) return (int)x->length - (int)y->length;
    return (int)x->symbol - (int)y->symbol;
}

/*
 * Generates canonical Huffman codes from tree
 * @param root : Root of Huffman tree
 * @param codes : Array of 256 to store generated codes
 */
void generate_canonical_codes(HuffmanNode *root, HuffmanCode *codes)
{
    memset(codes, 0, 256 * sizeof(HuffmanCode));

    unsigned char lengths[256] = {0};
    collect_lengths(root, lengths, 0);

    // collect only symbols that appear
    SymbolLength sl[256];
    int count = 0;
    for (int i = 0; i < 256; i++)
        if (lengths[i] > 0)
        {
            sl[count].symbol = (unsigned char)i;
            sl[count].length = lengths[i];
            count++;
        }

    // sort by length then symbol — canonical order
    qsort(sl, count, sizeof(SymbolLength), cmp_symbol_length);

    // assign canonical codes
    unsigned short code = 0;
    for (int i = 0; i < count; i++)
    {
        if (i > 0 && sl[i].length > sl[i-1].length)
            code <<= (sl[i].length - sl[i-1].length);  // shift up when length increases

        codes[sl[i].symbol].code   = code;
        codes[sl[i].symbol].length = sl[i].length;
        code++;
    }
}


// ─── HEADER ──────────────────────────────────────────────────────────────────

/*
 * Writes Huffman header (code lengths) to output
 * @param codes : Array of Huffman codes
 * @param output : Output buffer
 * @param out_len : Pointer to update with header size
 */
void write_header(HuffmanCode *codes, unsigned char *output, size_t *out_len, size_t original_len)
{
    for (int i = 0; i < 256; i++)
        output[i] = codes[i].length;

    // store original length in next 8 bytes
    for (int i = 0; i < 8; i++)
        output[256 + i] = (original_len >> (i * 8)) & 0xFF;

    *out_len = 264;
}

// ─── ENCODE DATA ─────────────────────────────────────────────────────────────

/*
 * Encodes data using generated codes
 * @param input : Input data
 * @param len : Length of input
 * @param codes : Huffman codes for each symbol
 * @param output : Output buffer
 * @param out_len : Pointer to update with encoded size
 */
void encode_data(unsigned char *input, size_t len, HuffmanCode *codes,
                 unsigned char *output, size_t *out_len)
{
    size_t  byte_idx  = 0;
    int     bit_pos   = 7;      // next bit position to write (MSB first)
    output[0]         = 0;

    for (size_t i = 0; i < len; i++)
    {
        unsigned short code   = codes[input[i]].code;
        unsigned char  length = codes[input[i]].length;

        for (int b = length - 1; b >= 0; b--)
        {
            if ((code >> b) & 1)
                output[byte_idx] |= (1 << bit_pos);

            bit_pos--;
            if (bit_pos < 0)
            {
                bit_pos = 7;
                output[++byte_idx] = 0;
            }
        }
    }

    // if last byte is partially written, count it
    *out_len = byte_idx + (bit_pos < 7 ? 1 : 0);
}


// ─── HUFFMAN ENCODE ───────────────────────────────────────────────────────────

/*
 * Encodes data using Huffman coding
 * @param input : Input byte array
 * @param len : Length of input
 * @param output : Output buffer for compressed data
 * @param out_len : Pointer to store output length
 */
void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len)
{
    // 1. count frequencies
    int frequencies[256] = {0};
    for (size_t i = 0; i < len; i++)
        frequencies[input[i]]++;

    // 2. build tree
    HuffmanNode *root = NULL;
    build_huffman_tree(frequencies, &root);

    // 3. generate canonical codes
    HuffmanCode codes[256];
    generate_canonical_codes(root, codes);

    // 4. write header (256 code lengths)
    size_t header_len = 0;
    write_header(codes, output, &header_len, len);

    // 5. encode data after header
    size_t data_len = 0;
    encode_data(input, len, codes, output + header_len, &data_len);

    *out_len = header_len + data_len;

    free_tree(root);
}


// ─── HUFFMAN DECODE ───────────────────────────────────────────────────────────

/*
 * Decodes Huffman encoded data
 * @param input : Huffman encoded data
 * @param len : Length of encoded data
 * @param output : Output buffer for decoded data
 * @param out_len : Pointer to store output length
 */
void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len)
{
    // 1. read code lengths
    unsigned char lengths[256];
    for (int i = 0; i < 256; i++)
        lengths[i] = input[i];

    // 2. read original length
    size_t original_len = 0;
    for (int i = 0; i < 8; i++)
        original_len |= ((size_t)input[256 + i]) << (i * 8);

    // 3. rebuild canonical codes
    SymbolLength sl[256];
    int count = 0;
    for (int i = 0; i < 256; i++)
        if (lengths[i] > 0)
        {
            sl[count].symbol = (unsigned char)i;
            sl[count].length = lengths[i];
            count++;
        }

    qsort(sl, count, sizeof(SymbolLength), cmp_symbol_length);

    HuffmanCode codes[256];
    memset(codes, 0, sizeof(codes));

    unsigned short code = 0;
    for (int i = 0; i < count; i++)
    {
        if (i > 0 && sl[i].length > sl[i-1].length)
            code <<= (sl[i].length - sl[i-1].length);
        codes[sl[i].symbol].code   = code;
        codes[sl[i].symbol].length = sl[i].length;
        code++;
    }

    // 4. decode bit by bit, stop at original_len
    size_t  byte_idx       = 264;   // start after header
    int     bit_pos        = 7;
    size_t  j              = 0;
    unsigned short current_code   = 0;
    unsigned char  current_length = 0;

    while (j < original_len)
    {
        int bit = (input[byte_idx] >> bit_pos) & 1;
        current_code   = (current_code << 1) | bit;
        current_length++;

        for (int i = 0; i < count; i++)
        {
            if (codes[sl[i].symbol].length == current_length &&
                codes[sl[i].symbol].code   == current_code)
            {
                output[j++]    = sl[i].symbol;
                current_code   = 0;
                current_length = 0;
                break;
            }
        }

        bit_pos--;
        if (bit_pos < 0)
        {
            bit_pos = 7;
            byte_idx++;
        }
    }

    output[j] = '\0';
    *out_len   = j;
}