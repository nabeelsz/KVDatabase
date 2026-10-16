#include "../src/hashmap.h"
#include <cassert> 
#include <cstdio> 
#include <string.h>
#include "testing.h"

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