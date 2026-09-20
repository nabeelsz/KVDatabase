#include <cstdint> 
#include <random> 
#include <pthread.h> 

#include <hashmap.h> 

const int MAX_ENTRIES = 1024; 

struct KVDatabase { 
    void init(); 

    // Size needed for memcpying of bytes 
    void Put(char* key, void* bytes, uint64_t size); 

    Result Get(char* key);

    HashMap kv_map; 
    pthread_mutex_t kv_lock; 
}; 