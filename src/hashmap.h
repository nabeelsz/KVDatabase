#include <cstdint>

const int MAX_ENTRIES = 1024; 

struct HashEntry {
    char* key; 
    void* data;
    uint64_t size; 
    HashEntry* next; 
}; 

struct Result {
    void* data; 
    uint64_t size; 
};

// Pre-allocated memory pool for KV insertion and chaining 
struct NodePool {
    NodePool(); 
    HashEntry* get_node(char* key, void* bytes, uint64_t size); 
    
    // Linked list for available nodes 
    HashEntry* curr_available; 
}; 

struct HashMap {
    ~HashMap(); 

    // Insert a KV-pair to the hashmap, the size is needed for memcpying of bytes. 
    int insert(char* key, void* bytes, uint64_t size); 
    // Return a KV pair's data, if the pair does not exist, then return size 0 and nullptr for data. 
    Result get(char* key); 

    HashEntry map[MAX_ENTRIES]; 
    NodePool pool; 
    int num_entries = 0; 
};

// Based on the FNV1-A Hash Algorithm: https://www.ietf.org/archive/id/draft-eastlake-fnv-22.html#name-fnv-offset_basis
int hash_func(char* key) {
    // 16777619
    uint32_t fnv_prime = 16777619; 
    uint32_t fnv_offset = 2166136261; 
    char* curr = key; 
    uint32_t hash = fnv_offset; 
    while (*curr != '\0') {
        hash = hash ^ (unsigned char)*curr; 
        hash = hash * fnv_prime;
        curr++; 
    }
    return hash % MAX_ENTRIES; 
}