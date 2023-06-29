#include "liburing.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define QUEUE_ENTRIES 1024
#define PORT 23333
#define MSGSIZE 200
#define TOTAL_SENDS 5000000
#define BATCHSIZE 8

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
    printf("batch size: %d\n", BATCHSIZE);
    struct io_uring ring;
    if (ring_setup(QUEUE_ENTRIES, &ring))
        return 1;

    int sockfd;
    struct sockaddr_in servaddr;
    if (sock_setup(&sockfd, &servaddr))
        return 1;

    char *some_data = malloc(MSGSIZE);
    struct io_uring_cqe cqes[BATCHSIZE];

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
                free(some_data);
                printf("cannot get sqe, this shouldn't happen in this case\n");
                return 1;
            }
            // setup SQE
            io_uring_prep_send(sqe, sockfd, some_data, MSGSIZE, 0);
            // memset(sqe, 0, sizeof(*sqe));
            // sqe->user_data = some_data;
            sqe->opcode = IORING_OP_SEND;
            // sqe->fd = sockfd;
            // // sqe->off = 0;  // offset, unused?
            // sqe->addr = (unsigned long)some_data;  // the buffer to write data
            // sqe->len = MSGSIZE;  // buffer length
        }
        
        int ret = io_uring_submit(&ring);
        if (ret < 0)  // should not happen
            fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));

        ret = io_uring_peek_batch_cqe(&ring, &cqes, BATCHSIZE);
        for (int k = 0; k < ret; k++)
        {
            io_uring_cqe_seen(&ring, &(cqes[k]));
        }

            // while (true)
            // {
            //     struct io_uring_cqe *cqe;
            //     ret = io_uring_peek_cqe(&ring, &cqe);
            //     // ret = io_uring_wait_cqe(&ring, &cqe); // pointer to pointer here!

            //     // should not happen here, but see possible errors for `io_uring_enter`
            //     // this can be related to queue overflow mentioned above
            //     if (ret < 0)
            //         break;
            //     io_uring_cqe_seen(&ring, cqe);
            // }
        
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
    struct timespec result = diff(time1, time2);
    printf("total time: %d:%d\n", result.tv_sec, result.tv_nsec);

    // we can batch the SQEs, or submit 1 by 1 like above
    // batching adds delay to processing the corresponding CQEs
    // may become a problem if queue length too short,
    // or see details for IORING_FEAT_NODROP
    // int ret = io_uring_submit(&ring);
    // if (ret < 0) // should not happen
    //     fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
    // else
    //     printf("total submitted: %d\n", ret);

        // using the library util function wait_cqe here, may customise it later
    
    // we know there are 10 CQE, or we can use peek instead of wait
    // which is like non-blocking
    // for (int i = 0; i < 10; i++)
    // {
    //     struct io_uring_cqe *cqe;
    //     ret = io_uring_wait_cqe(&ring, &cqe);  // pointer to pointer here!

    //     // should not happen here, but see possible errors for `io_uring_enter`
    //     // this can be related to queue overflow mentioned above
    //     if (ret < 0)
    //         fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));

    //     // return value/error just like a system call is in ->res
    //     // printf("sent bytes: %d\n", cqe->res);
        
    //     io_uring_cqe_seen(&ring, cqe); // IMPORTANT, dequeue it
    // }
    free(some_data); // if there's any user data, free it
    close(sockfd);
    io_uring_queue_exit(&ring);
    printf("closing and exit\n");
    return 0;
}