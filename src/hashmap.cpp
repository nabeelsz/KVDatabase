#include <hashmap.h> 
#include <cstdlib>
#include <string.h>
#include <cassert> 

enum ErrCodes {
    SUCCESS, 
    FAILURE = -1
};

// Allocate the node pool to be used for the hashmap entries in the form of a linekd list. 
NodePool::NodePool() {
    int num_allocated = 0; 
    HashEntry* curr = nullptr; 
    while (num_allocated < MAX_ENTRIES) {
        HashEntry* curr = (HashEntry*)malloc(sizeof(HashEntry)); 
        *(curr->key) = '\0'; 
        curr->size = -1; 
        curr->next = nullptr; 
        curr_available = curr; 
        // If first allocation then modify head variable to point to the first allocated
        if (num_allocated == 0) curr_available = curr; 
        num_allocated++; 
        curr = curr->next; 
    }
}

// Get a node from the node pool with relevant metadata for it to be inserted. 
HashEntry* NodePool::get_node(char* key, void* bytes, uint64_t size) {
    HashEntry* returned_node = curr_available; 
    strcpy(returned_node->key, key); 
    memcpy(returned_node->data, bytes, size); 
    returned_node->size = size; 
    // Increment head to next valid entry
    curr_available = curr_available->next; 

    return returned_node; 
}

HashMap::~HashMap() {
    // Delete all allocations consisting from the node pool as well as entries in the map as well 

    // Firstly delete nodes from the linked list
    HashEntry* curr_node = pool.curr_available; 
    while (curr_node) {
        HashEntry* next = curr_node->next; 
        delete curr_node; 
        curr_node = next; 
    }

    // Now delete the allocated nodes in the map. 
    for (int i = 0; i < MAX_ENTRIES; i++) {
        curr_node = &map[i]; 
        while (curr_node) {
            HashEntry* next = curr_node->next; 
            delete curr_node; 
            curr_node = next; 
        }
    }
}

// TODO: Maybe make insert return an optional to indicate success? 
int HashMap::insert(char* key, void* bytes, uint64_t size) {
    if (num_entries == MAX_ENTRIES) return FAILURE; 

    int hash = hash_func(key); 
    HashEntry* new_entry = pool.get_node(key, bytes, size); 
    HashEntry* curr_node = &map[hash]; 
    HashEntry* prev_node = nullptr; 
    // Iterate until we have found an empty entry to insert to 
    while (curr_node) {
        prev_node = curr_node; 
        curr_node = curr_node->next; 
    }
    assert(!curr_node && prev_node); 
    curr_node = new_entry; 
    prev_node->next = curr_node; 
    num_entries++; 

    return SUCCESS; 
}

// TODO: Maybe return an optional to indicate success/failure? 
Result HashMap::get(char* key) {
    int hash = hash_func(key); 
    HashEntry* current_node = &map[hash]; 
    while (current_node) {
        if (strcmp(key, current_node->key) == 0) break; 
        current_node = current_node->next; 
    }
    
    return current_node ? Result{data: current_node->data, size: current_node->size} : Result{data: nullptr, size: 0};
}