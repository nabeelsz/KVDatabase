#include <cstdint> 
#include <random> 
#include <pthread.h> 

#include <hashmap.h> 

const int MAX_ENTRIES = 1024; 

struct KVDataBase { 
    // Insert a KV-pair to the database, the size is needed for memcpying of bytes. 
    void put(char* key, void* bytes, uint64_t size); 
    // Return a KV pair's data, if the pair does not exist, then return size -1 and nullptr for data. 
    Result get(char* key);

    HashMap kv_map; 
    pthread_mutex_t kv_lock; 
}; 