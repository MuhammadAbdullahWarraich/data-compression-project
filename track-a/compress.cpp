#include "find_match.h" 
#include <iostream>

void output() {}

template <typename T, typename... Tail>
void output(T arg, Tail... more_args) {
    // TODO: replace cout with writing to a file
    std::cout << arg;
    output(more_args...);
}

template <typename T, typename... Tail>
void compress(char* input, size_t input_size) {
    Match<T> match;
    size_t i = 0;
    while (i < input_size) {
        if (find_match(i, input, input_size, match)) {
            output(1, match.match_len, match.distance);
            i += match.match_len;
        } else {
            output(0, input[i]);
            i += 1;
        }
    }
}
