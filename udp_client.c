#include "common.h"

static void* udp_client_test(void* param) {
    int sock_fd;
    struct sockaddr* pservaddr;
    
    pservaddr = (struct sockaddr*)param;
    
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    
    int nbytes;
    char sendbuf[MAXBUF + 1], recvbuf[MAXBUF + 1];
    
    bzero(recvbuf, MAXBUF + 1);
    bzero(sendbuf, MAXBUF + 1);
    
    if (connect(sock_fd, (struct sockaddr *)pservaddr, sizeof(*pservaddr)) == -1) {
        perror("connect error");
        exit(1);
    }
    sprintf(sendbuf, "%d%d%s", 1258, 4578, "HelloWorld!");
    write(sock_fd, sendbuf, strlen(sendbuf));
    
    nbytes = read(sock_fd, recvbuf, MAXBUF);
    if (nbytes < 0) {
        perror("read error");
    }
    printf("recv %s  %dfrom server on thread %ld \n", recvbuf, nbytes, pthread_self());
    
    close(sock_fd);
    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    struct sockaddr_in servaddr;
    int i = 0;
    
    /*check args*/
    if (argc != 2) {
        printf("usage: udpclient <addr> \n");
        exit(1);
    }
    
    /* init servaddr */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("[%s] is not a valid address\n", argv[1]);
        exit(1);
    }
    
    pthread_t threads[CLIENT_THREAD_COUNT];
    pthread_attr_t attrs[CLIENT_THREAD_COUNT];
    int ret_val[CLIENT_THREAD_COUNT];
    
    for (i = 0; i < CLIENT_THREAD_COUNT; ++i) {
        pthread_attr_init(&attrs[i]);
                         
        if (pthread_create(&threads[i], &attrs[i], (void *)udp_client_test, &servaddr)) {
            perror("pthread create error");
            exit(-1);
        }
    }
    
    for (i = 0; i < CLIENT_THREAD_COUNT; ++i) {
        pthread_join(threads[i], (void *)&ret_val[i]);
    }
}
