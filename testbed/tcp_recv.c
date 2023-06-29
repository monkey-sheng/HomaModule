#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    int recv_size;
    char *buffer = malloc(1024);

    // Create a TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a specific address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(23333);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 23333...\n");

    // Accept incoming connections
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("Received connection\n");

    struct timespec time1, time2;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);

    int l = 0;
    while (l < 5000000)
    {
        l += 1;
        // Receive data from the client
        recv_size = recv(client_socket, buffer, 1, 0);
        if (recv_size < 0)
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
    }
       

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
    // printf("Received %d bytes: %s\n", recv_size, buffer);
    struct timespec result = diff(time1, time2);
    printf("total time: %d:%d\n", result.tv_sec, result.tv_nsec);
    // Close the sockets
    close(client_socket);
    close(server_socket);

    return 0;
}

