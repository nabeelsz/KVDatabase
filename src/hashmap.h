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

// Pre-allocated pool for chaining 
struct NodePool {
    void init(int num_allocate); 
    HashEntry* get_node(char* key, void* bytes, uint64_t size); 
    // Linked list for available nodes 
    HashEntry* curr_available; 
}; 

struct HashMap {
    void init(); 
    void insert(char* key, void* bytes, uint64_t size); 
    Result get(char* key); 

    HashEntry map[MAX_ENTRIES]; 
    NodePool pool; 
    int num_entries = 0; 
};