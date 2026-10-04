
#include <cstdio>
#include <cstdint> 
#include <cstring>
// socket libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 

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


bool run_op(char operation[CMD_LENGTH], char key[MAX_STRING_LENGTH], void* value, int size, KVDataBase* database, 
    Result*& get_result) 
    {
    if (strcmp(operation, "PUT\0") == 0) {
        int success = database->put(key, value, size); 
        if (success == FAILURE) return false;
        return true;  
    }
    else if (strcmp(operation, "GET\0") == 0) {
        *get_result = database->get(key); 
        printf("Result size = %d\n", get_result->size); 
        if (get_result->size == FAILURE) return false; 
        return true; 
    }

    // Return false, should be an unreachable path. 
    return false; 
}

void client_response(int client_fd, char cmd[CMD_LENGTH], void* value, int size, bool success) {
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
        int bytes_sent = send(client_fd, (void*)(response + total_sent), num_bytes - total_sent, MSG_NOSIGNAL); 
        if (bytes_sent == -1) {
            printf("ERROR: Client closed socket.\n"); 
            return; 
        }
        total_sent += bytes_sent; 
    }

    // Send over GET value if op is a GET with the same methodology used in the response communication. 
    if (strcmp(cmd, "GET\0") == 0) {
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
void request_handler(char client_req[MAX_MSG_BYTES], KVDataBase* database, int client_fd) {
    char cmd[CMD_LENGTH + 1]; 
    void* value; 

    // Copy over the command + add a null terminator and verify it's valid. 
    memcpy(cmd, client_req, CMD_LENGTH); 
    cmd[CMD_LENGTH] = '\0'; 
    bool is_put = strcmp(cmd, "PUT\0") == 0; 
    if (!is_put && strcmp(cmd, "GET\0") != 0) {
        printf("ERROR: Operation is not a PUT or GET\n"); 
        return;
    }
    // Need a space between CMD and Key. 
    if (client_req[3] != ' ') {
        return; 
    }

    // For parsing over the key.
    int num_read = 0; 
    int key_idx = 0; 
    const int KEY_OFFSET = 4; 
    
    // Parse over the key until we reach the end of it, and then memcpy it if it's valid. 
    while (num_read < MAX_STRING_LENGTH) {
        // Space for PUT or null terminator/newline for GET meaning that there is no more of the key to read. 
        if (client_req[num_read + KEY_OFFSET ] == ' ' || client_req[num_read + KEY_OFFSET] == '\0' || 
            client_req[num_read + KEY_OFFSET] == '\n') {
            break; 
        }
        num_read++;  
    }
    // If we've reached the end of the maximum length of the key and there's still more then mark it as invalid. 
    if (num_read == MAX_STRING_LENGTH && client_req[num_read + KEY_OFFSET] != ' ') {
        return;
    }
    printf("Num read = %d\n", num_read); 
    char key[num_read + NULL_TERMINATOR_SIZE]; 
    memcpy(key, (void*)(client_req + KEY_OFFSET), num_read); 
    key[num_read + NULL_TERMINATOR_SIZE - 1] = '\0'; 

    // If the command is a PUT, get the size of the value in bytes and then memcpy it. 
    int num_size = 0; 
    if (is_put) {
        // Begin at the first byte of the value.
        const int VALUE_OFFSET = KEY_OFFSET + num_read + 1; 
        while (num_size < MAX_STRING_LENGTH) {
            int value_idx = num_size + VALUE_OFFSET; 
            if (client_req[value_idx] == '\0' || client_req[value_idx] == '\n') {
                // Include null terminator in the data being stored. 
                if (client_req[value_idx] == '\0') {
                    num_size++; 
                }
                break; 
            }
            num_size++; 
        }
        if (num_size == MAX_STRING_LENGTH && (client_req[num_size + VALUE_OFFSET] != '\0' 
            || client_req[num_size + VALUE_OFFSET] != '\n')) {
            return;
        }
        value = malloc(sizeof(char) * num_size); 
        memcpy(value, (void*)&client_req[VALUE_OFFSET], num_size);  
    }
    Result* get_result = (Result*)malloc(sizeof(Result)); 
    bool op_success = run_op(cmd, key, value, num_size, database, get_result); 
    printf("Op success = %d\n", op_success); 

    // // Now communicate with the client
    // client_response(client_fd, cmd, value, num_size, op_success);  

    free(get_result); 
    if (is_put) {
        free(value); 
    }
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
    while ((bytes_received = recv(client_fd, (void*)(client_buf + num_bytes_read), MAX_MSG_BYTES - num_bytes_read, 0)) >= -1) {
        if (bytes_received == 0) {
            printf("Client closed socket.\n"); 
            return NULL; 
        }
        else if (bytes_received == -1) {
            printf("Error occurred regarding socket.\n");
            return NULL; 
        }
        if (num_bytes_read + bytes_received > MAX_MSG_BYTES) { 
            printf("Invalid message: too large.\n"); 
            close(client_fd); 
            return NULL;  
        }
        if (client_buf[bytes_received] == '\0' || client_buf[bytes_received] == '\n') {
            break; 
        }
        num_bytes_read =+ bytes_received; 
    }

    request_handler(client_buf, database, client_fd); 
    
    return NULL; 
}

// int main(int argc, char *argv[]) {
//     KVDataBase* database = (KVDataBase*)malloc(sizeof(KVDataBase)); 

//     // Socket code:
//     // check if a port is supplied, otherwise use a random available port (through port 0)
//     int port = 0;
//     if (argc == 2) {
//         port = atoi(argv[1]);
//     }
//     // Create a socket
//     int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (socket_fd == -1) {
//         printf("ERROR: Issue creating socket.\n");
//     }
//     // Upon successful creation, bind the socket to an address
//     struct sockaddr_in address = {0}; 
//     address.sin_family = AF_INET; 
//     address.sin_port = htons(port); 
//     address.sin_addr.s_addr = INADDR_ANY; 
//     if (bind(socket_fd, (sockaddr *)&address, sizeof(address)) == -1) {
//         printf("ERROR: Issue binding socket.\n");
//     }
//     // Now mark the socket as listenable so it can receive requests from clients
//     int max_connections = 128;
//     listen(socket_fd, max_connections);
//     // Now accept incoming requests from clients
//     while (true)
//     {
//         socklen_t addr_size = sizeof(address); 
//         int connected_fd = accept(socket_fd, (sockaddr *)&address, &addr_size);
//         if (connected_fd == -1)
//         {
//             printf("ERROR: Issue retrieving connection file descriptor from accept\n");
//         }
//         ThreadArgs args = {connected_fd, database}; 
//         pthread_t placeholder; 
//         if (pthread_create(&placeholder, nullptr, &thread_func, (void*)&args) != 0) {
//             printf("ERROR: Error with creating thread.\n");
//         }
//     }
// }