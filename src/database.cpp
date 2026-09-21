#include <database.h> 

int KVDataBase::put(char* key, void* bytes, uint64_t size) {
    pthread_mutex_lock(&kv_lock); 
    int err_code = kv_map.insert(key, bytes, size); 
    pthread_mutex_unlock(&kv_lock); 
    return err_code; 
}

Result KVDataBase::get(char* key) {
    pthread_mutex_lock(&kv_lock); 
    Result value = kv_map.get(key); 
    pthread_mutex_unlock(&kv_lock);
    return value; 
}

