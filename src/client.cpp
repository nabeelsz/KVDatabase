
#include <cstdio> 
// socket libraries
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char* argv[]) {
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
   if (bind(socket_fd, (sockaddr*)&address, sizeof(address)) == -1) {
    printf("ERROR: Issue binding socket.\n"); 
   } 
   // Now mark the socket as listenable so it can receive requests from clients
   int max_connections = 128; 
   listen(socket_fd, max_connections); 
   // Now accept incoming requests from clients 
   while (true) {
        int connected_fd = accept(socket_fd, (sockaddr*)&address, (socklen_t*)sizeof(address)); 
        if (connected_fd == -1) {
            printf("ERROR: Issue retrieving connection file descriptor from accept\n"); 
        }
        // TODO: create a function that interacts with the client (handles request) and spawn a thread for it. 
   } 

}