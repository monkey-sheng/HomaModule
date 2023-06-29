#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <time.h>
#define TOTAL_SENDS 5000000

/* 
 * First, setup ring before we can use it
 */
// static int ring_setup(unsigned int entries, struct io_uring *ring)
// {
//     // This liburing function handles all the setup and mmap'ing needed for the
//     // SQ/CQ rings and sqe/cqe arrays
//     int ret = io_uring_queue_init(entries, ring, 0);
//     if (ret < 0)
//     {
//         fprintf(stderr, "queue_init: %s\n", strerror(-ret));
//         return -1;
//     }

//     return 0;
// }

/* 
 * UDP sender setup
 */
int sock_setup(int *sockfd, struct sockaddr_in *servaddr)
{
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = INADDR_ANY;
    servaddr->sin_port = htons(23333);

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

int main(int argc, char *argv[])
{
    // struct io_uring ring;
    // if (ring_setup(QUEUE_ENTRIES, &ring))
    //     return 1;

    int sockfd;
    struct sockaddr_in servaddr;
    if (sock_setup(&sockfd, &servaddr))
        return 1;

    
    void *some_data = malloc(200);

    struct timespec time1, time2;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
    for (int i = 1; i <= TOTAL_SENDS; i++)
    {
        int ret = send(sockfd, some_data, 200, 0);
        // printf("sent udp: %d\n", ret);
        // if (ret < 0) // should not happen
        //     fprintf(stderr, "send: %d\n", ret);
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);

    struct timespec result = diff(time1, time2);

    close(sockfd);
    printf("total time: %d:%d\n", result.tv_sec, result.tv_nsec);
    return 0;
}