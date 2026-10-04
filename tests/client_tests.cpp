// Client.cpp tests. 
#include <client.cpp> 
#include <cassert>  

// Tests valid input 
void req_handler_tests() {
    KVDataBase test_db;
    char req1[] = "PUT key test\0"; 
    request_handler(req1, &test_db, -1);        
    Result result_1 = test_db.get("key");
    printf("Key-value pair size of value = %d\n", result_1.size);
    assert(result_1.size == 5); 
    char* res1_val = (char*)result_1.data; 
    assert(strcmp(res1_val, "test\0") == 0); 

    // GET test on the same key
    char req2[] = "GET key\n"; 
    request_handler(req2, &test_db, -1); 

    // test maximum size key 
    char req3[] = "PUT 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef 1\n";
    request_handler(req3, &test_db, -1); 
    Result result_3 = test_db.get("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    assert(result_3.size == 1); 
    char* res3_val = (char*)result_3.data; 
    assert(strcmp(res3_val, "1\0") == 0);

}

int main() {
    req_handler_tests(); 
    return 0; 
}