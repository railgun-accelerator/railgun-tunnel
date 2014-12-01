#include <common.h>
#include <utils.h>

#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
    struct sockaddr_in servaddr;
    void *data_buffer, *iter_buf;
    int i = 0, nfds = 0, nbytes = 0, seq = 0, data_read_size = 0;
    int data_fd = 0, kdp_fd = 0, sock_fd = 0;
    long filelength = 0;
    struct epoll_event ev;
    struct epoll_event events[MAXCLIENTEPOLLSIZE];
    PAYLOAD_PACKET pay_load;
    char recvbuf[MAXBUF + 1];
    
    /*check args*/
    if (argc != 3) {
        printf("usage: udpclient <addr> <data file>\n");
        exit(1);
    }
    
    /* init servaddr */
    bzero(recvbuf, MAXBUF + 1);
    bzero(&servaddr, sizeof(servaddr));
    bzero(&pay_load, sizeof(PAYLOAD_PACKET));
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
    iter_buf = data_buffer = mmap(NULL, filelength, PROT_READ, MAP_PRIVATE, data_fd, 0);
    
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (connect(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("connect error");
        exit(1);
    }
    
    set_non_blocking(sock_fd);
    
    kdp_fd = epoll_create(MAXCLIENTEPOLLSIZE);
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sock_fd;
    
    epoll_ctl(kdp_fd, EPOLL_CTL_ADD, sock_fd, &ev);
    while (1) {
        nfds = epoll_wait(kdp_fd, events, 10000, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            goto error;
        }

        for (i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                nbytes = read(sock_fd, recvbuf, MAXBUF);
                if (nbytes < 0) {
                    if (errno != EAGAIN) {
                        perror("read error");
                    }
                } else {
                    u_int32_t ack = ntohl(*(u_int32_t *)&recvbuf[0]);
                    printf("recv %d from server \n", ack);
                }
            } else if(events[i].events & EPOLLOUT) {
                if (data_read_size + PAYLOAD_SIZE <= filelength) {
                    pay_load.ack = 0;
                    pay_load.seq = seq++;
                    int packet_skip_size = PAYLOAD_SIZE;
                    bzero(&pay_load.data, PAYLOAD_SIZE);
                    memcpy(&pay_load.data, iter_buf, packet_skip_size);
                    payload_htonl(&pay_load);
                    nbytes = write(sock_fd, &pay_load, sizeof(PAYLOAD_PACKET));
                    if (nbytes < 0) {
                        if (errno != EAGAIN) {
                            perror("write error");
                        }
                    }
                    printf("send %d to server  \n", nbytes);
                    data_read_size += packet_skip_size;
                    iter_buf += packet_skip_size;
                } else {
                    printf("done. \n");
                    goto error;
                }
            }
        }
    }
error:
    munmap(data_buffer, filelength);
    close(data_fd);
    close(sock_fd);
    exit(0);
}
