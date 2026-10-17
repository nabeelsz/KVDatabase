#include <database.h> 
#include <time.h>

int KVDataBase::put(char* key, void* bytes, int size) {
    bool expected = false; 
    bool desired = true; 
    while (!locked.compare_exchange_weak(expected, desired)){
        expected = false; 
    }
    int err_code = kv_map.insert(key, bytes, size); 
    locked.store(false);  
    return err_code; 
}

Result KVDataBase::get(char* key) {
    bool expected = false; 
    bool desired = true; 
    // TODO: Replace this with a custom implemented RWlock instead? No need to strictly serialize every read 
    while (!locked.compare_exchange_weak(expected, desired)) {
        expected = false; 
    }
    Result value = kv_map.get(key); 
    locked.store(false);
    return value; 
}

