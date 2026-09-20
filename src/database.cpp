#include <database.h> 

void KVDataBase::put(char* key, void* bytes, uint64_t size) {
    pthread_mutex_lock(&kv_lock); 
    kv_map.insert(key, bytes, size); 
    pthread_mutex_unlock(&kv_lock); 

}

Result KVDataBase::get(char* key) {
    pthread_mutex_lock(&kv_lock); 
    Result value = kv_map.get(key); 
    pthread_mutex_unlock(&kv_lock);
    return value; 
}

