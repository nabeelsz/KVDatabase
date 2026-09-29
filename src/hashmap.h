/*
    Hashmap.h 
    Contains the hash map class that supports basic functions from insertion and getting a KV pair's value. To support this, 
    a node pool is pre-allocated so that there is zero dynamic memory allocation, and then nodes from the pool are used to 
    populate the hash map. 
*/

#include <cstdint>

const int MAX_ENTRIES = 1024; 
// Maximum string length in bytes. This is used to cap the largest string a key can be, as well as the value. 
const int MAX_STRING_LENGTH = 256; 

enum ErrCodes {
    SUCCESS, 
    FAILURE = -1
};
struct HashEntry {
    char* key; 
    void* data;
    uint64_t size; 
    HashEntry* next; 
}; 

struct Result {
    void* data; 
    int size; 
};

// Pre-allocated memory pool for KV insertion and chaining 
struct NodePool {
    // Allocate the node pool to be used for the hashmap entries in the form of a linekd list. 
    NodePool(); 

    // Get a node from the node pool with relevant metadata for it to be inserted. 
    HashEntry* get_node(char* key, void* bytes, uint64_t size); 
    
    // Linked list for available nodes 
    HashEntry* curr_available; 
}; 

struct HashMap {
    // Initialize all entries to be empty and not containing anything. 
    HashMap(); 

    // Deallocate all of the node pool allocations that are currently in the pool or in the hashmap. 
    ~HashMap(); 

    // Insert a KV-pair to the hashmap, the size is needed for memcpying of bytes. 
    int insert(char* key, void* bytes, int size); 

    // Return a KV pair's data, if the pair does not exist, then return size 0 and nullptr for data. 
    Result get(char* key); 

    HashEntry* map[MAX_ENTRIES]; 
    NodePool pool; 
    int num_entries = 0; 
};

// Based on the FNV1-A Hash Algorithm: https://www.ietf.org/archive/id/draft-eastlake-fnv-22.html#name-fnv-offset_basis
int hash_func(char* key);