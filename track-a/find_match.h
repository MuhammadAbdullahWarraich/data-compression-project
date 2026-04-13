#ifndef FIND_MATCH_HEADER
#define FIND_MATCH_HEADER

#include "compression_config.h"
#include <cstddef>
#include <limits>

#define MATCH_THRESHOLD 3

#define is_valid_find_match_T(T)                            \
    static_assert(                                          \
        std::numeric_limits<T>::max() > BACK_SCAN_LIMIT     \
            &&                                              \
        std::numeric_limits<T>::is_integer                  \
            &&                                              \
        !std::numeric_limits<T>::is_signed                  \
    );                                                      \

template <typename T>
struct Match {

    is_valid_find_match_T(T);

    T match_len;
    T distance;
};

template <typename T>
bool find_match(size_t i, char* input, size_t input_size, Match<T>& match);


#endif
