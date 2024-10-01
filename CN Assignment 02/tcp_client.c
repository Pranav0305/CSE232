#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>

#define PORT 9110

void *clientThreadHandler(void *args)
{
    // Create socket.
    int network_socket;
    network_socket = socket(AF_INET,SOCK_STREAM,0);

    // Specify address at socket.
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Connect to socket.
    if(connect(network_socket,(struct sockaddr *) &server_address, sizeof(server_address)) == -1)
    {
        perror("Failed to connect to remote socket");
        exit(1);
    }

    // Send message to server.
    char msg[125] = "Hello from client";
    if(send(network_socket, msg, sizeof(msg), 0) == -1)
    {
        perror("Failed to send message");
        exit(1);
    }

    // Receive data from server.
    char server_response[1024];
    if(recv(network_socket,&server_response, sizeof(server_response), 0) == -1)
    {
        perror("Failed to receive server message");
        exit(1);
    }

    printf("Server: %s\n",server_response);

    // Close socket connection.
    close(network_socket);
}

int main(int argc, char* argv[])
{
    int connections =  atoi(argv[1]);
    pthread_t clientThreads[connections];
    for(int i = 0; i < connections; i++)
    {
        pthread_create(&clientThreads[i], NULL, clientThreadHandler, NULL);
    }

    for(int i = 0; i < connections; i++)
    {
        pthread_join(clientThreads[i], NULL);
    }

}
