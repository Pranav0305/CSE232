#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    printf("Enter message: ");
    char *message;
    size_t msgSize = 0;
    getline(&message,&msgSize,stdin);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address / Address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server. Sending message...\n");

    // Send message to the server
    send(sock, message, strlen(message), 0);
    printf("Message sent\n");

    // Receive message from the server
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0)
    {
        buffer[valread] = '\0'; // Null-terminate the received data
        printf("Server replied: %s\n", buffer);
    }

    close(sock);
    return 0;
}
