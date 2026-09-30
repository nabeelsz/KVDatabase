
#include <cstdio>
#include <cstdint> 
#include <cstring>
// socket libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 

#include "hashmap.h"
#include "database.h"

// Max CMD length (PUT or GET)
const int CMD_LENGTH = 3; 
const int NULL_TERMINATOR_SIZE = 1; 
// Maximum total message bytes = length of command + largest possible length of key + largest possible length of value + null 
// terminator byte. 
const int MAX_MSG_BYTES = CMD_LENGTH + MAX_STRING_LENGTH * 2 + NULL_TERMINATOR_SIZE;  

// struct to pass into pthread creation 
struct ThreadArgs {
    int client_fd; 
    KVDataBase* database; 
}; 


bool run_op(char operation[CMD_LENGTH], char key[MAX_STRING_LENGTH], void* value, uint64_t size, KVDataBase* database, 
    Result*& get_result) 
    {
    if (strcmp(operation, "PUT") == 0) {
        int success = database->put(key, value, size); 
        if (success == FAILURE) return false; 
    }
    else if (strcmp(operation, "GET") == 0) {
        Result value = database->get(key); 
        if (value.size == FAILURE) return false; 
        *get_result = value; 
    }

    // Return true if no failures above ran
    return true; 
}

void client_response(int client_fd, char cmd[CMD_LENGTH], void* value, uint64_t size, bool success) {
    const char* response; 
    if (success) {
        response = "Success!\n"; 
    }
    else {
        response = "Failure, error encountered\n"; 
    }
    int total_sent = 0; 
    int num_bytes = strlen(response); 

    // Send over the response to the client, incrementing the pointer based on the number of bytes we have sent, ensuring onl
    // the bytes we haven't sent are sent over. 
    while (total_sent < num_bytes) {
        int bytes_sent = send(client_fd, (void*)response + total_sent, num_bytes - total_sent, MSG_NOSIGNAL); 
        if (bytes_sent == -1) {
            printf("ERROR: Client closed socket.\n"); 
            return; 
        }
        total_sent += bytes_sent; 
    }

    // Send over GET value if op is a GET with the same methodology used in the response communication. 
    if (strcmp(cmd, "GET") == 0) {
        total_sent = 0; 
        while (total_sent < size) {
            int bytes_sent = send(client_fd, value + total_sent, size - total_sent, MSG_NOSIGNAL); 
            if (bytes_sent == -1) {
                printf("ERROR: Client closed socket.\n"); 
                return; 
            }
            total_sent += bytes_sent; 
        }
    }

    close(client_fd); 
}

// Handles the request by firstly parsing it, and then running it on the database, returning true if the request was successfully
// ran or false if the request was malformed or if the request was not successfully ran in the database. 
bool request_handler(char client_req[MAX_MSG_BYTES], KVDataBase* database, int client_fd) {
    char cmd[CMD_LENGTH]; 
    char key[MAX_STRING_LENGTH + NULL_TERMINATOR_SIZE]; 
    void* value; 

    // First copy over first 3 bytes.
    memcpy(cmd, client_req, CMD_LENGTH); 
    bool is_put = strcmp(cmd, "PUT") == 0; 
    if (!is_put && strcmp(cmd, "GET") != 0) {
        printf("ERROR: Operation is not a PUT or GET\n"); 
        return false; 
    }
    // Need a space between CMD and Key. 
    if (client_req[3] != ' ') {
        return false; 
    }

    // Parse over the key.
    int num_read = 0; 
    int key_idx = 0; 
    const int KEY_OFFSET = 3; 
    
    while (num_read < MAX_STRING_LENGTH) {
        // Space meaning that there is no more of the key to read. 
        if (client_req[num_read + KEY_OFFSET ] == ' ') {
            key[key_idx++] = '\0'; 
        }
        key[key_idx++] = client_req[num_read + KEY_OFFSET];
        num_read++;  
    }

    // If we've reached the end of the maximum length of the key and there's still more then mark it as invalid. 
    if (num_read == MAX_STRING_LENGTH && client_req[num_read + KEY_OFFSET] != ' ') {
        return false; 
    }

    // If the command is a PUT, get the size of the value in bytes and then memcpy it. 
    int num_size = 0; 
    if (is_put) {
        const int VALUE_OFFSET = KEY_OFFSET + num_read; 
        while (num_size < MAX_STRING_LENGTH) {
            int value_idx = num_size + VALUE_OFFSET; 
            if (client_req[value_idx] == '\0' || client_req[value_idx] == '\n') {
                break; 
            }
            num_size++; 
        }
        if (num_size == MAX_STRING_LENGTH && (client_req[num_size + VALUE_OFFSET] != '\0' 
            || client_req[num_size + VALUE_OFFSET] != '\n')) {
            return false; 
        }
        memcpy(value, (void*)client_req[VALUE_OFFSET], num_size);  
    }
    Result* get_result; 
    bool op_success = run_op(cmd, key, value, num_size, database, get_result); 

    // Now communicate with the client
    client_response(client_fd, cmd, value, num_size, op_success);  
}

// Supported format type: GET <string> or PUT <string> <arbitrary value><\0 or \n>
// Note that the arbitrary value's maximum size is 512 bytes. 
// A valid request needs to have a null terminator or endline character so the server knows when to stop reading.  
void* thread_func(void* thread_args) {
    // Construct arguments  
    ThreadArgs* args = (ThreadArgs*)thread_args;
    int client_fd = args->client_fd; 
    KVDataBase* database = args->database;
    
    // Used for parsing client requests
    char client_buf[MAX_MSG_BYTES]; 
    int num_bytes_read = 0; 
    int bytes_received;

    // Keep reading until we reach the end of the request, or the request is too large, or the client closes the socket.  
    while (bytes_received = recv(client_fd, (void*)client_buf + num_bytes_read, MAX_MSG_BYTES - num_bytes_read, 0)) {
        if (bytes_received == 0) {
            printf("Client closed socket.\n"); 
            return; 
        }
        if (num_bytes_read + bytes_received > MAX_MSG_BYTES) { 
            printf("Invalid message: too large.\n"); 
            close(client_fd); 
            return;  
        }
        if (client_buf[bytes_received] == '\0' || client_buf[bytes_received] == '\n') {
            break; 
        }
        num_bytes_read =+ bytes_received; 
    }

    request_handler(client_buf, database, client_fd); 
    
    return NULL; 
}

int main(int argc, char *argv[]) {
    KVDataBase* database = (KVDataBase*)malloc(sizeof(KVDataBase)); 

    // Socket code:
    // check if a port is supplied, otherwise use a random available port (through port 0)
    int port = 0;
    if (argc == 2) {
        port = (int)argv[1];
    }
    // Create a socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("ERROR: Issue creating socket.\n");
    }
    // Upon successful creation, bind the socket to an address
    struct sockaddr_in address;
    if (bind(socket_fd, (sockaddr *)&address, sizeof(address)) == -1) {
        printf("ERROR: Issue binding socket.\n");
    }
    // Now mark the socket as listenable so it can receive requests from clients
    int max_connections = 128;
    listen(socket_fd, max_connections);
    // Now accept incoming requests from clients
    while (true)
    {
        int connected_fd = accept(socket_fd, (sockaddr *)&address, (socklen_t *)sizeof(address));
        if (connected_fd == -1)
        {
            printf("ERROR: Issue retrieving connection file descriptor from accept\n");
        }
        // TODO: create a function that interacts with the client (handles request) and spawn a thread for it.
        ThreadArgs args = {connected_fd, database}; 
        if (pthread_create(nullptr, nullptr, &thread_func, (void*)&args) == -1) {
            printf("ERROR: Error with creating thread.\n");
        }
    }
}