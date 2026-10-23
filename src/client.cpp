#include <cstdio>
#include <cstdint> 
#include <cstring>
// socket libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <sys/resource.h> 
#include <sys/epoll.h> 
#include <errno.h> 

#include "database.h"
#include "debug.h"

// Max CMD length (PUT or GET)
const int CMD_LENGTH = 3; 
const int SPACE_OFFSET = 1; 
// Maximum total message bytes = length of command + space inbetween 
// +  largest possible length of key + space inbetween + largest possible length of value + null terminator byte. 
const int MAX_MSG_BYTES = CMD_LENGTH + MAX_STRING_LENGTH * 2 + NULL_TERMINATOR_SIZE + (2 * SPACE_OFFSET);  

// struct to pass into pthread creation 
struct ThreadArgs {
    int client_fd; 
    KVDataBase* database; 
    char client_buf[MAX_MSG_BYTES]; 
}; 
struct ConnectionState {
    char client_buf[MAX_MSG_BYTES]; 
    int num_bytes_read = 0; 
    int bytes_received;
};

bool run_op(char operation[CMD_LENGTH + NULL_TERMINATOR_SIZE], char key[MAX_STRING_LENGTH + NULL_TERMINATOR_SIZE], void* value, int size, 
    KVDataBase* database, Result& get_result) 
    {
    if (strcmp(operation, "PUT\0") == 0) {
        int success = database->put(key, value, size); 
        if (success == FAILURE) return false;
        return true;  
    }
    else if (strcmp(operation, "GET\0") == 0) {
        get_result = database->get(key); 

        #if DEBUG
        printf("Result size = %d\n", get_result.size);
        #endif 

        size = get_result.size;  
        if (get_result.size == FAILURE) return false; 
        return true; 
    }

    // Return false, should be an unreachable path. 
    printf("ERROR: Reached an unreachable path, neither a PUT or a GET was completed.\n"); 
    return false; 
}

void client_response(int client_fd, char cmd[CMD_LENGTH + NULL_TERMINATOR_SIZE], bool success, 
    Result get_result) {
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
        while (total_sent < get_result.size) {
            int bytes_sent = send(client_fd, get_result.data + total_sent, get_result.size - total_sent, MSG_NOSIGNAL); 
            if (bytes_sent == -1) {
                printf("ERROR: Client closed socket.\n"); 
                return; 
            }
            total_sent += bytes_sent; 
        }
    }

    close(client_fd); 
}

bool cmd_parser(char client_req[MAX_MSG_BYTES], char** cmd) {
    // Copy over the command + add a null terminator and verify it's valid. 
    memcpy(*cmd, client_req, CMD_LENGTH); 
    (*cmd)[CMD_LENGTH] = '\0'; 

    #if DEBUG
    printf("Test, cmd = %s\n", *cmd); 
    #endif

    if (strcmp((*cmd), "PUT\0") != 0 && strcmp((*cmd), "GET\0") != 0) {
        printf("ERROR: Operation is not a PUT or GET\n"); 
        return false;
    }
    // Need a space between CMD and Key. 
    if (client_req[3] != ' ') {
        printf("ERROR: No space inbetween the key and the command\n"); 
        return false; 
    }
    return true; 
}

bool key_parser(char client_req[MAX_MSG_BYTES], int& num_read, const int CMD_OFFSET, bool is_put, bool is_get) {
    while (num_read < MAX_STRING_LENGTH) {
        // Space for PUT or null terminator/newline for GET meaning that there is no more of the key to read. 
        if (client_req[num_read + CMD_OFFSET ] == ' ' || client_req[num_read + CMD_OFFSET] == '\0' || 
            client_req[num_read + CMD_OFFSET] == '\n') {
            break; 
        }
        num_read++;  
    }
    // If we've reached the end of the maximum length of the key and there's still more for then mark it as invalid. 
    if ((is_put) && ((num_read == MAX_STRING_LENGTH && client_req[num_read + CMD_OFFSET] != ' ')) || 
        ((is_get) && client_req[num_read + CMD_OFFSET] != '\0')) {
        printf("ERROR: Key is too larger than the supported maximum key size.\n"); 
        return false;
    }
    return true; 
}

bool value_parser(char client_req[MAX_MSG_BYTES], int& num_size, const int VALUE_OFFSET) {
    // Begin at the first byte of the value.
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
        && client_req[num_size + VALUE_OFFSET] != '\n')) {
            printf("ERROR: Value is larger than the maximum supported value size. The size of value is %d\n", num_size);
            printf("Value at 256th byte is %c\n", client_req[num_size + VALUE_OFFSET]);
            return false;
    }
    // Include null terminator if we've reached the max string length, since we would've exited out of the loop without 
    // reading it. 
    if (num_size == MAX_STRING_LENGTH && (client_req[num_size + VALUE_OFFSET] == '\0')) {
        num_size++; 
    }
    
    return true; 
}

// Handles the request by firstly parsing it, and then running it on the database, returning true if the request was successfully
// ran or false if the request was malformed or if the request was not successfully ran in the database. 
void request_handler(char client_req[MAX_MSG_BYTES], KVDataBase* database, int client_fd) {
    char cmd[CMD_LENGTH + NULL_TERMINATOR_SIZE]; 
    char* ptr = cmd; 

    if (!cmd_parser(client_req, &ptr)) {
        return; 
    } 
    bool is_put = (strcmp(cmd, "PUT\0") == 0); 
    bool is_get = (strcmp(cmd, "GET\0") == 0); 

    // Parse over the key and get its size, then memcpy the key part of the client's request into the key variable.  
    int num_read = 0; 
    const int CMD_OFFSET = CMD_LENGTH + SPACE_OFFSET; 
    if (!key_parser(client_req, num_read, CMD_OFFSET, is_put, is_get)) {
        return; 
    }

    #if DEBUG
    printf("Num read = %d\n", num_read);
    #endif  

    char key[num_read + NULL_TERMINATOR_SIZE]; 
    memcpy(key, (void*)(client_req + CMD_OFFSET), num_read); 
    key[num_read + NULL_TERMINATOR_SIZE - 1] = '\0'; 

    // If PUT, then parse over the value. 
    int num_size = 0; 
    const int KEY_OFFSET = num_read + SPACE_OFFSET; 
    // Begin at the first byte of the value.
    const int VALUE_OFFSET = CMD_OFFSET + KEY_OFFSET; 

    if (is_put) {
       if (!value_parser(client_req, num_size, VALUE_OFFSET)) {
            return; 
       } 
    }
    else if (is_get) {
        // Dummy number to avoid a runtime error of a VLA being 0, doesn't matter since value isn't used. 
        num_size = 1; 
    }
    char value[num_size]; 
    memcpy(value, (void*)&client_req[VALUE_OFFSET], num_size);

    // Run the operation after parsing the client request. 
    Result get_result; 
    bool op_success = run_op(cmd, key, (void*)value, num_size, database, get_result); 

    #if DEBUG
    printf("Op success = %d\n", op_success); 
    #endif 

    // Now communicate with the client to send success response back and potentially data. 
    client_response(client_fd, cmd, op_success, get_result);  
}

// A trampoline function used to cleanly pass in argument variables. This will eventually be removed. 
void* request_trampoline(void* thread_args) {
    ThreadArgs* args = (ThreadArgs*)thread_args;
    request_handler(args->client_buf, args->database, args->client_fd);
    // TODO: Implement a thread pool that maps file descriptor -> corresponding worker thread to deal with a fix sized allocation 
    // of threads and memory allocation, and to remove dynamic allocation. 
    free(args); 
    return NULL; 
}

// Supported format type: GET <string> or PUT <string> <arbitrary value><\0 or \n>
// Note that the arbitrary value's maximum size is 512 bytes. 
// A valid request needs to have a null terminator or endline character so the server knows when to stop reading.  
// TODO: Create new helper function for the epoll parsing loop
void receive_message(KVDataBase* database, int epoll_fd, int client_fd, ConnectionState* state, epoll_event* ev) {
    bool deinitialize = false; 
    state[client_fd].bytes_received = 
    recv(
        client_fd, 
        (void*)(state[client_fd].client_buf + state[client_fd].num_bytes_read), 
        MAX_MSG_BYTES - state[client_fd].num_bytes_read, 
        0);
    state[client_fd].num_bytes_read += state[client_fd].bytes_received;   

    #if DEBUG
        printf("%.*s\n", state[client_fd].bytes_received, state[client_fd].client_buf + state[client_fd].num_bytes_read);
    #endif 

    // No errors, check if done parsing what the client sent. 
    if (state[client_fd].bytes_received > 0 && 
        (state[client_fd].num_bytes_read <= MAX_MSG_BYTES)) {
        // Reached end of receiving client request, create a thread to now parse this request and potentially run it. 
        if (state[client_fd].client_buf[state[client_fd].num_bytes_read - 1] == '\0' 
            || state[client_fd].client_buf[state[client_fd].num_bytes_read - 1] == '\n') {
            deinitialize = true; 
            ThreadArgs* args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
            args->client_fd = client_fd; 
            args->database = database;
            memcpy(args->client_buf, state[client_fd].client_buf, sizeof(char) * state[client_fd].num_bytes_read);  
            pthread_t id; 
            pthread_create(&id, NULL, &request_trampoline, (void*)(args));  
        }
    }
    // If there is an error regarding the data sent or attempting to receive it. 
    else {
        if (state[client_fd].bytes_received == 0) {
            printf("Error: Client closed socket for file descriptor %d.\n", client_fd); 
            close(client_fd); 
        } 
        else if (state[client_fd].bytes_received == -1) {
            int err_val = errno; 
            printf("Error occurred regarding socket %d, errno value: %d.\n", client_fd, err_val);
        }
        else if (state[client_fd].num_bytes_read > MAX_MSG_BYTES) {
            printf("Invalid message: too large.\n"); 
            close(client_fd); 
        }
        deinitialize = true; 
    }
    // Reset state to not provide false data if a future connection reoccupies this file descriptor. 
    if (deinitialize) {
        memset(state[client_fd].client_buf, 0, sizeof(char) * MAX_MSG_BYTES);
        state[client_fd].num_bytes_read = 0; 
        state[client_fd].bytes_received = 0; 
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, ev) == -1) {
            printf("Error: Epoll deletion of client fd %d failed.\n", client_fd); 
        }
    }
}


int main(int argc, char *argv[]) {
    KVDataBase database; 

    // Socket code:
    // check if a port is supplied, otherwise use a random available port (through port 0)
    int port = 0;
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    // Create a socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("ERROR: Issue creating socket.\n");
        return -1; 
    }
    #if DEBUG
        printf("Socket initialized for server = %d.\n", socket_fd); 
    #endif 
    // Upon successful creation, bind the socket to an address
    struct sockaddr_in address = {0}; 
    address.sin_family = AF_INET; 
    address.sin_port = htons(port); 
    address.sin_addr.s_addr = INADDR_ANY; 
    if (bind(socket_fd, (sockaddr *)&address, sizeof(address)) == -1) {
        printf("ERROR: Issue binding socket.\n");
        return -1; 
    }
    // Now mark the socket as listenable so it can receive requests from clients
    int max_connections = 128;
    listen(socket_fd, max_connections);

    // Initialize an epoll file descriptor and add the listening socket to the epoll file descriptor list
    ConnectionState state[RLIMIT_NOFILE]; 
    int epoll_fd = epoll_create(RLIMIT_FSIZE); 
    if (epoll_fd == -1) {
        printf("ERROR: Issue getting epoll file descriptor.\n"); 
        return -1; 
    }
    epoll_event ev, events[RLIMIT_FSIZE]; 
    ev.events = EPOLLIN; 
    // epoll_wait's events structure returns integers representing the ready file descriptor(s) number, but it needs to be 
    // supplied. 
    ev.data.fd = socket_fd; 
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
        printf("Error: Error adding server's socket to epoll instance.\n"); 
        return -1; 
    }

    while (true) {
        int num_ready = epoll_wait(epoll_fd, events, RLIMIT_FSIZE, -1); 
        if (num_ready == -1) {
            printf("Error: Error with epoll_wait.\n"); 
            return -1; 
        }
        for (int i = 0; i < num_ready; i++) {
            int client_fd = events[i].data.fd; 
            #if DEBUG
                printf("Client file descriptor from epoll ready list = %d.\n", client_fd); 
                printf("Client fd == socket fd = %d.\n", client_fd == socket_fd);
                printf("Socket fd initialized for server = %d.\n", socket_fd);  
            #endif 
            // Incoming client request to create a connection with the server
            if (client_fd == socket_fd) {
                socklen_t addr_size = sizeof(address); 
                int connected_fd = accept(socket_fd, (sockaddr *)&address, &addr_size);
                 if (connected_fd == -1) {
                    printf("ERROR: Issue retrieving connection file descriptor from accept\n");
                    continue; 
                }
                ev.data.fd = connected_fd; 
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connected_fd, &ev) == -1) {
                    printf("Error adding file descriptor to epoll list.\n"); 
                    continue; 
                }
            }
            // Client sending over data through a socket 
            else {
                receive_message(&database, epoll_fd, client_fd, state, &ev);
                printf("test2\n");
            }
        }
    }

    close(socket_fd);
    close(epoll_fd);  
    return 0; 
}