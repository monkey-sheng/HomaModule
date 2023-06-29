#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include "liburing.h"

#define QUEUE_ENTRIES 1024
#define PORT 23333
#define MSGSIZE 100
#define TOTAL_SENDS 5000000
#define BATCHSIZE 2

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

static int ring_setup(unsigned int entries, struct io_uring *ring)
{
    // This liburing function handles all the setup and mmap'ing needed for the
    // SQ/CQ rings and sqe/cqe arrays
    int ret = io_uring_queue_init(entries, ring, 0);
    if (ret < 0)
    {
        fprintf(stderr, "queue_init: %s\n", strerror(-ret));
        return -1;
    }

    return 0;
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    int recv_size;
    char *buffer = malloc(1024);
    struct io_uring_cqe cqes[BATCHSIZE];

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

    printf("batch size: %d\n", BATCHSIZE);
    struct io_uring ring;
    if (ring_setup(QUEUE_ENTRIES, &ring))
        return 1;

    struct timespec time1, time2;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);

    int i = 0;
    while (i < TOTAL_SENDS)
    {
        for (int j = 0; j < BATCHSIZE; j++)
        {
            i += 1;
            struct io_uring_sqe *sqe;
            sqe = io_uring_get_sqe(&ring);
            if (!sqe)
            {
                printf("cannot get sqe, this shouldn't happen in this case\n");
                return 1;
            }
            // setup SQE
            io_uring_prep_recv(sqe, client_socket, buffer, MSGSIZE, 0);
            // memset(sqe, 0, sizeof(*sqe));
            // sqe->user_data = some_data;
            // sqe->opcode = IORING_OP_SEND;
            // sqe->fd = sockfd;
            // // sqe->off = 0;  // offset, unused?
            // sqe->addr = (unsigned long)some_data;  // the buffer to write data
            // sqe->len = MSGSIZE;  // buffer length
        }

        int ret = io_uring_submit(&ring);
        if (ret < 0) // should not happen
            fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));

        ret = io_uring_peek_batch_cqe(&ring, (struct io_uring_cqe**)&cqes, BATCHSIZE);
        for (int k = 0; k < ret; k++)
        {
            io_uring_cqe_seen(&ring, &(cqes[k]));
        }
        // // Receive data from the client
        // recv_size = recv(client_socket, buffer, 100, 0);
        // if (recv_size < 0)
        // {
        //     perror("recv");
        //     exit(EXIT_FAILURE);
        // }
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

