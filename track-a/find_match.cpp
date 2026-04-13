#include "find_match.h"
#include "compression_config.h"
#include <vector> 
using std::vector;

// if c++ doesn't obey orphan rule, make this a method maybe
template <typename T>
void insert_into_vec(vector<T>& v, size_t idx, T&& val) {
    if (v.size() < idx) v.resize(idx + 1);// TODO: find a better way
    v[idx] = val;
}

template <typename T>
bool find_match(size_t i, char* input, size_t input_size, Match<T>& match) {

    is_valid_find_match_T(T);

    if (input_size - i < MATCH_THRESHOLD) return false;

    T search_buffer_size = std::min(static_cast<T>(i), BACK_SCAN_LIMIT);
    if (search_buffer_size < MATCH_THRESHOLD) return false;

    vector<T> lps;
    lps.push_back(0);

    char* pattern = input + i;
    char* search_buffer = input - search_buffer_size;

    T i = 0, len = 0;
    T max_val = 0;
    T max_val_idx = 0;

    while (i < search_buffer_size) {
        if (search_buffer[i] == pattern[len]) {
            len += 1;
            insert_into_vec(lps, i, len);
        } else if (len == 0) {
            insert_into_vec(lps, i, 0);
        } else {
            len = lps[len-1];
        }
        if (max_val < lps[len]) {
            max_val_idx = i;
            max_val = lps[len];
        }
    }

    if (max_val > MATCH_THRESHOLD) {
        match.match_len = max_val;
        match.distance = search_buffer_size - max_val_idx;
        return true;
    }
    return false;

}
