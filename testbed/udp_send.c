#include "liburing.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
 * UDP sender setup
 */
int sock_setup(int *sockfd, struct sockaddr_in *servaddr)
{
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = INADDR_ANY;
    servaddr->sin_port = htons(PORT);

    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd < 0)
        return 1;
    
    if (connect(*sockfd, (struct sockaddr *)servaddr, sizeof(*servaddr)) < 0)
    {
        perror("connect failed");
        return 1;
    }
    else
        return 0;
}

int main(int argc, char *argv[])
{
    struct io_uring ring;
    if (ring_setup(QUEUE_ENTRIES, &ring))
        return 1;

    int sockfd;
    struct sockaddr_in servaddr;
    if (sock_setup(&sockfd, &servaddr))
        return 1;

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
        // setup SQE
        memset(sqe, 0, sizeof(*sqe));
        sqe->user_data = some_data;
        sqe->opcode = IORING_OP_SEND;
        sqe->fd = sockfd;
        // sqe->off = 0;  // offset, unused?
        sqe->addr = (unsigned long)some_data;  // the buffer to write data
        sqe->len = i;  // buffer length

        // int ret = io_uring_submit(&ring);
        // if (ret < 0)  // should not happen
        //     fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
    }

    // we can batch the SQEs, or submit 1 by 1 like above
    // batching adds delay to processing the corresponding CQEs
    // may become a problem if queue length too short,
    // or see details for IORING_FEAT_NODROP
    int ret = io_uring_submit(&ring);
    if (ret < 0) // should not happen
        fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
    else
        printf("total submitted: %d\n", ret);

        // using the library util function wait_cqe here, may customise it later
    
    // we know there are 11 CQE, or we can use peek instead of wait
    // which is like non-blocking
    for (int i = 0; i < 11; i++)
    {
        struct io_uring_cqe *cqe;
        ret = io_uring_wait_cqe(&ring, &cqe);  // pointer to pointer here!

        // should not happen here, but see possible errors for `io_uring_enter`
        // this can be related to queue overflow mentioned above
        if (ret < 0)
            fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));

        // return value/error just like a system call is in ->res
        printf("sent bytes: %d\n", cqe->res);
        free(cqe->user_data);  // if there's any user data, free it
        io_uring_cqe_seen(&ring, cqe); // IMPORTANT, dequeue it
    }
    close(sockfd);
    io_uring_queue_exit(&ring);
    printf("closing and exit\n");
    return 0;
}