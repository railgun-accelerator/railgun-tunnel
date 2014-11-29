#include <common.h>
#include <utils.h>

#include <sys/mman.h>
#include <sys/stat.h>


typedef struct udp_client_param {
    struct sockaddr* paddr;
    void* data_src;
    long data_length;
} UDP_CLIENT_PARAM;

static void* udp_client_test(void* param) {
    int sock_fd, data_read_size = 0, seq = 0;
    UDP_CLIENT_PARAM* ucp;
    ucp = (UDP_CLIENT_PARAM*)param;
    PAYLOAD_PACKET pay_load;
    int nbytes;
    void* data_buf;
    char recvbuf[MAXBUF + 1];
    
    bzero(&pay_load, sizeof(PAYLOAD_PACKET));
    bzero(recvbuf, MAXBUF + 1);
    data_buf = ucp->data_src;
    
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (connect(sock_fd, (struct sockaddr *)ucp->paddr, sizeof(*ucp->paddr)) == -1) {
        perror("connect error");
        exit(1);
    }

    while(data_read_size + PAYLOAD_SIZE <= ucp->data_length) {
        pay_load.ack = 0;
        pay_load.seq = seq++;
        int packet_skip_size = PAYLOAD_SIZE;
        bzero(&pay_load.data, PAYLOAD_SIZE);
 //       if (unlikely(ucp->data_length - data_read_size < PAYLOAD_SIZE)) {
 //           packet_skip_size = ucp->data_length - data_read_size;
 //       }
        
        memcpy(&pay_load.data, data_buf, packet_skip_size);
        
        payload_htonl(&pay_load);
        nbytes = write(sock_fd, &pay_load, sizeof(PAYLOAD_PACKET));
        if (nbytes < 0) {
            perror("write error");
            pthread_exit(NULL);
        }
        printf("send %d to server on thread %ld \n", nbytes, pthread_self());
        nbytes = read(sock_fd, recvbuf, MAXBUF);
        if (nbytes < 0) {
            perror("read error");
        }
        u_int32_t ack = ntohl(*(u_int32_t *)&recvbuf[0]);
        printf("recv %d  %d from server on thread %ld \n", ack, nbytes, pthread_self());
        data_read_size += packet_skip_size;
        data_buf += packet_skip_size;
    }
    close(sock_fd);
    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    struct sockaddr_in servaddr;
    UDP_CLIENT_PARAM param;
    void* data_buffer;
    int i = 0;
    int data_fd = 0;
    long filelength = 0;
    
    /*check args*/
    if (argc != 3) {
        printf("usage: udpclient <addr> <data file>\n");
        exit(1);
    }
    
    /* init servaddr */
    bzero(&servaddr, sizeof(servaddr));
    bzero(&param, sizeof(UDP_CLIENT_PARAM));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("[%s] is not a valid address\n", argv[1]);
        exit(1);
    }
    
    /* init data area*/
    data_fd = open(argv[2], O_RDONLY);
    if (data_fd < 0) {
        printf("open data file failed : %s", strerror(errno));
        exit(1);
    }
    filelength = lseek(data_fd, 1, SEEK_END);
    lseek(data_fd, 0, SEEK_SET);
    data_buffer = mmap(NULL, filelength, PROT_READ, MAP_PRIVATE, data_fd, 0);
    param.paddr = (struct sockaddr*)&servaddr;
    param.data_src = data_buffer;
    param.data_length = filelength;

    
    pthread_t threads[CLIENT_THREAD_COUNT];
    pthread_attr_t attrs[CLIENT_THREAD_COUNT];
    int ret_val[CLIENT_THREAD_COUNT];
    
    for (i = 0; i < CLIENT_THREAD_COUNT; ++i) {
        pthread_attr_init(&attrs[i]);
                         
        if (pthread_create(&threads[i], &attrs[i], (void *)udp_client_test, &param)) {
            perror("pthread create error");
            exit(-1);
        }
    }
    
    for (i = 0; i < CLIENT_THREAD_COUNT; ++i) {
        pthread_join(threads[i], (void *)&ret_val[i]);
    }
    
    munmap(data_buffer, filelength);
    close(data_fd);
    exit(0);
}
