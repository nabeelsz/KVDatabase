/*
    database.h
    This header file contains the class implementation of a database that supports key-value pairs. It currently supports an 
    arbitrary amount of 1024 entries. 
*/
#include <cstdint> 
#include <random> 
#include <pthread.h> 

#include <hashmap.h> 

struct KVDataBase { 
    KVDataBase() {
        pthread_mutex_init(&kv_lock, nullptr); 
    }
    ~KVDataBase() {
        pthread_mutex_destroy(&kv_lock); 
    }
    // Insert a KV-pair to the database, the size is needed for memcpying of bytes. 
    int put(char* key, void* bytes, int size); 
    // Return a KV pair's data, if the pair does not exist, then return size -1 and nullptr for data. 
    Result get(char* key);

    HashMap kv_map; 
    pthread_mutex_t kv_lock; 
}; 