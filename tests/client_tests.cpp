// Client.cpp tests. 
#include <client.cpp> 
#include <cassert>  

// Tests valid input 
// TODO: May be better to do memcmp instead of strcmp

// NOTE: Need to turn off socket code for these tests to run. 
void req_handler_tests() {
    KVDataBase test_db;
    // // TESTS below
    // Basic PUT request test
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

    // // test maximum size key 
    // char req3[] = "PUT 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef 1\0";
    // request_handler(req3, &test_db, -1); 
    // Result result_3 = test_db.get("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    // assert(result_3.size == 2); 
    // char* res3_val = (char*)result_3.data; 
    // printf("Res_3 val = %c\n", *res3_val); 
    // assert(strcmp(res3_val, "1\0") == 0);

    // test maximum size value 
    char max_size_val[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\0"; 
    char req4[] = "PUT key1 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\0";
    request_handler(req4, &test_db, -1); 
    Result result_4 = test_db.get("key1"); 
    printf("Size = %d\n", result_4.size); 
    assert(result_4.size == MAX_STRING_LENGTH + 1);
    assert(result_4.data); 
    char* res4_val = (char*)result_4.data; 
    assert(strcmp(max_size_val, res4_val) == 0); 

    // Test GET command 
    char req5[] = "GET key1\0"; 
    request_handler(req5, &test_db, -1); 

    // Test PUT for max size key AND val AND their both the same string 
    // TODO: Decide if I want to support multiple PUTS with different values for the same key, if so, then I can uncomment
    // the 3rd test. Otherwise, you can only run one of the two. 
    char req6[] = "PUT 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\0"; 
    request_handler(req6, &test_db, -1); 
    Result result_6 = test_db.get(max_size_val); 
    char* res6_data = (char*)result_6.data; 
    printf("Result 6 size = %d\n", result_6.size); 
    assert(result_6.size == MAX_STRING_LENGTH + NULL_TERMINATOR_SIZE); 
    assert(strcmp(max_size_val, res6_data) == 0); 
}

// TODO: Test socket functionality and full end-to-end

int main() {
    req_handler_tests(); 
    return 0; 
}