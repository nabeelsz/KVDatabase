// Client.cpp tests. 
#include <client.cpp> 
#include <cassert>  

void req_handler_tests() {
    KVDataBase test_db;
    char req1[] = "PUT key test\0"; 
    request_handler(req1, &test_db, -1);        
    Result result_1 = test_db.get("key");
    printf("Key-value pair size of value = %d\n", result_1.size);
    assert(result_1.size == 4); 
    char* res1_val = (char*)result_1.data; 
    assert(strcmp(res1_val, "test\0") == 0); 
}

int main() {
    req_handler_tests(); 
    return 0; 
}