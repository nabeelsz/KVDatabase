#include <hashmap.h> 
#include <cstdlib>
#include <string.h>
#include <cassert> 
#include <cstdio> 

enum ErrCodes {
    SUCCESS, 
    FAILURE = -1
};

NodePool::NodePool() {
    int num_allocated = 0; 
    HashEntry dummy_entry = HashEntry{nullptr, 0x0, 0, nullptr}; 
    HashEntry* prev = &dummy_entry; 
    while (num_allocated < MAX_ENTRIES) {
        HashEntry* curr = (HashEntry*)malloc(sizeof(HashEntry)); 
        curr->key = (char*)malloc(sizeof(char) * MAX_STRING_LENGTH); 
        // allocating an arbitrary number of bytes, TODO: decide maximum number of bytes we want to support, also is there a way
        // to avoid fragmentation? since some values may be ints which take up less bytes. Need to investigate.
        curr->data = malloc(sizeof(char) * 64); 
        curr->size = 0; 
        curr->next = nullptr; 
        // If first allocation then modify head variable to point to the first allocated
        if (num_allocated == 0) curr_available = curr; 
        num_allocated++; 
        prev->next = curr; 
        prev = curr; 
        curr = curr->next; 
    }
}

// Get a node from the node pool with relevant metadata for it to be inserted. 
HashEntry* NodePool::get_node(char* key, void* bytes, uint64_t size) {
    HashEntry* returned_node = curr_available; 
    strcpy(returned_node->key, key);
    memcpy(returned_node->data, bytes, size); 
    assert(returned_node->data); 
    assert(returned_node->key); 
    returned_node->size = size; 
    // Increment head to next valid entry
    curr_available = curr_available->next; 
    return returned_node; 
}

HashMap::HashMap() {
    // Initialize all entries to be nullptr, TODO: Maybe see if I can just memset it to be all nullptr instead of doing it in a loop?
    for (int i = 0; i < MAX_ENTRIES; i++) {
        map[i] = nullptr; 
    }
}

HashMap::~HashMap() {
    // Firstly delete nodes from the linked list
    HashEntry* curr_node = pool.curr_available; 
    while (curr_node) {
        HashEntry* next = curr_node->next; 
        free(curr_node->key); 
        free(curr_node->data);
        free(curr_node); 
        curr_node = next; 
    }

    // Now delete the allocated nodes in the map. 
    for (int i = 0; i < MAX_ENTRIES; i++) {
        curr_node = map[i]; 
        while (curr_node) {
            HashEntry* next = curr_node->next; 
            num_entries--; 
            free(curr_node->key); 
            free(curr_node->data); 
            free(curr_node); 
            curr_node = next; 
        }
    }
    assert(num_entries == 0); 
}

int HashMap::insert(char* key, void* bytes, uint64_t size) {
    if (num_entries == MAX_ENTRIES) return FAILURE; 
    int hash = hash_func(key);
    HashEntry* new_entry = pool.get_node(key, bytes, size); 
    // Next pointer is populated due to node pool so set it to nullptr
    // TODO: maybe instead of having the node pool be a linked 
    // list, it can instead be an array of allocated pointers to avoid having to do this. 
    new_entry->next = nullptr; 
    HashEntry** curr_node = &map[hash];
    HashEntry dummy_entry = HashEntry{nullptr, 0x0, 0, nullptr}; 
    HashEntry* prev_node = &dummy_entry;  
    // Iterate until we have found an empty entry to insert to 
    // TODO: See if there's an easier way to insert without having to have a pointer to a pointer, maybe just change what the 
    // array pointer points to?
    while (*curr_node) {
        prev_node = *curr_node; 
        assert(*curr_node != (*curr_node)->next); 
        curr_node = &((*curr_node)->next);
    }
    *curr_node = new_entry; 
    prev_node->next = *curr_node; 
    num_entries++; 
    assert((*curr_node)->size == size); 
    return SUCCESS; 
}

Result HashMap::get(char* key) {
    int hash = hash_func(key); 
    HashEntry* current_node = map[hash]; 
    while (current_node) {
        if (strcmp(key, current_node->key) == 0) break; 
        current_node = current_node->next; 
    }
    return current_node ? Result{data: current_node->data, size: current_node->size} : Result{data: nullptr, size: 0};
}

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