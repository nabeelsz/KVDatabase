#include <cassert> 
#include <random> 
#include <cstdio> 
#include <string.h>

// Generates a random null-terminated C-string of a specified length
// Length includes null terminator. 
void generate_random_cstring(char* buffer, std::size_t length) {
    if (length == 0 || buffer == nullptr) return;

    // Define the character pool to pick from
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    // sizeof(charset) - 1 excludes the null terminator of the charset string literal
    const std::size_t max_index = sizeof(charset) - 1;

    // Set up the modern C++ random number generator engine
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> dist(0, max_index - 1); // Uniform distribution over indices

    // Populate the buffer with random characters
    for (std::size_t i = 0; i < length; ++i) {
        buffer[i] = charset[dist(gen)];
    }

    // Explicitly add the null terminator at the end of the C-string
    buffer[length] = '\0';
}