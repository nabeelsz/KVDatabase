#include "../src/hashmap.h"
#include <cassert> 
#include <random> 
#include <cstdio> 
#include <string.h>

// Generates a random null-terminated C-string of a specified length
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

void check_pool_allocation() {
    std::printf("Starting pool allocation tests.\n"); 
    HashMap map; 
    assert(map.num_entries == 0); 
    HashEntry* current_node = map.pool.curr_available; 
    assert(map.pool.curr_available); 
    int num_entries = 0; 
    while (current_node) {
        num_entries++; 
        assert(current_node->size == 0); 
        current_node = current_node->next; 
    }
    assert(num_entries == MAX_ENTRIES); 
    std::printf("Pool allocation tests passed.\n"); 
}

// NODE POOL TESTS
void node_pool_tests() { 
    check_pool_allocation(); 
}


void hash_map_insert_tests() {
    HashMap map; 
    std::printf("Starting insert and get tests.\n"); 
    for (int i = 0; i < MAX_ENTRIES; i++) {
        int string_length = 26; 
        char random_str[string_length + 1]; 
        generate_random_cstring(random_str, string_length); 
        map.insert(random_str, (void*)(random_str), sizeof(char) * string_length + 1); 
        Result kv_result = map.get(random_str);
        char* data_str = (char*)(kv_result.data); 
        assert(kv_result.data); 
        assert(data_str); 
        assert(strcmp(data_str, random_str) == 0); 
    }
    assert(map.num_entries == MAX_ENTRIES); 
    std::printf("Finished insert and get tests.\n"); 
}

// HASH MAP TESTS
void hash_map_tests() {
    hash_map_insert_tests(); 
}


int main() {
    node_pool_tests(); 
    hash_map_tests(); 
    return 0; 
}