#include "liburing.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <homa.h>

#define QUEUE_ENTRIES 32
#define PORT 2333

/*
 * First, setup ring before we can use it
 */
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

/*
 * Homa sender setup TODO
 */
int sock_setup(int *sockfd, struct sockaddr_in *servaddr)
{
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = INADDR_ANY;
    servaddr->sin_port = htons(PORT);

    *sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_HOMA);
    if (*sockfd < 0)
        return 1;
    else
        return 0;
}

int main(int argc, char *argv[])
{
    struct io_uring ring;
    if (ring_setup(QUEUE_ENTRIES, &ring))
    {
        printf("ring setup failed\n");
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    if (sock_setup(&sockfd, &servaddr))
    {
        printf("socket setup failed\n");
        return 1;
    }

    for (int i = 1; i <= 1024; i *= 2)
    {
        char *some_data = malloc(i);
        struct io_uring_sqe *sqe;
        sqe = io_uring_get_sqe(&ring);
        if (!sqe)
        {
            free(some_data);
            printf("cannot get sqe, this shouldn't happen in this case\n");
            return 1;
        }
        // setup SQE, we can do it manually, or choose one of the util methods
        
        memset(sqe, 0, sizeof(*sqe));
        sqe->user_data = some_data;
        sqe->opcode = IORING_OP_SEND;
        sqe->fd = sockfd;
        // sqe->off = 0;  // offset, unused?
        sqe->addr = (unsigned long)some_data; // the buffer to write data
        sqe->len = i;                         // buffer length

        // int ret = io_uring_submit(&ring);
        // if (ret < 0)  // should not happen
        //     fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
    }
}
