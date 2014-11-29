#include <udp_server.h>
#include <utils.h>

/**
 * set non blocking
 */

 static int set_non_blocking(int sockfd) {
     if (fcntl(sockfd, F_SETFL,
      fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
         return -1;
     }
     return 0;
 }

/**
 * handle message
 */
 void* pthread_handle_message(int* sock_fd) {
     char sendbuf[MAXBUF + 1];
     PAYLOAD_PACKET packet;
     int ret;
     int new_fd;
     
     struct sockaddr_in client_addr;
     socklen_t cli_len = sizeof(client_addr);
     
     new_fd = *sock_fd;
     
     /*start handle*/
     bzero(&packet, sizeof(PAYLOAD_PACKET));
     bzero(sendbuf, MAXBUF + 1);
     
     /*recv client message*/
     
     ret = recvfrom(new_fd, &packet, sizeof(PAYLOAD_PACKET), 0, (struct sockaddr*)&client_addr, &cli_len);
     if (ret > 0) {
          printf("socket %d recv from %s:%d message: %d bytes\n",
           new_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ret);
     }
     //payload_ntohl(&packet);
     memcpy(sendbuf, &packet, sizeof(uint32_t));
     memcpy(&sendbuf[4], &packet, sizeof(uint32_t));
     
     ret = sendto(new_fd, sendbuf, 2 * sizeof(uint32_t), 0, (struct sockaddr*)&client_addr, cli_len);
     
     if (ret < 0) {
         printf("send message error: %s", strerror(errno));
     }
     fflush(stdout);
     pthread_exit(NULL);
 }
 
 int main(int argc, char** argv) {
     int listen_fd, kdp_fd, nfds, n;
     struct sockaddr_in my_addr;
     struct epoll_event ev;
     struct epoll_event events[MAXEPOLLSIZE];
     struct rlimit rt;
     
     
     pthread_t thread;
     pthread_attr_t attr;
     
     /*set max opend fd*/
     rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
     if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
         perror("setrilimit");
         exit(1);
     }
     
     /*start listen*/
     if ((listen_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
         perror("socket create failed !");
         exit(1);
     }
     
     /*set socket opt, port reusable*/
     int opt = SO_REUSEADDR;
     setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
     set_non_blocking(listen_fd);
     
     bzero(&my_addr, sizeof(struct sockaddr_in));
     my_addr.sin_family = PF_INET;
     my_addr.sin_port = htons(SERV_PORT);
     my_addr.sin_addr.s_addr = INADDR_ANY;
     
     if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
         perror("bind failed!");
         exit(1);
     }
     
     /*create epoll handle, add listen socket to epoll set*/
     kdp_fd = epoll_create(MAXEPOLLSIZE);
     ev.events = EPOLLIN|EPOLLET;
     ev.data.fd = listen_fd;
     if (epoll_ctl(kdp_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
         fprintf(stderr, "epoll set insertion error: fd = %d/n", listen_fd);
     }
     
     while(1) {
         nfds = epoll_wait(kdp_fd, events, 10000, -1);
         if (nfds == -1) {
             perror("epoll_wait");
             break;
         }
         
         for(n = 0; n < nfds; ++n) {
             if(events[n].data.fd == listen_fd) {
                 pthread_attr_init(&attr);
                 pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
                 pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                 
                 if (pthread_create(&thread, &attr, (void *)pthread_handle_message, (void*)&(events[n].data.fd))) {
                     perror("pthread create error");
                     exit(-1);
                 }
             }
         }
     }
     
     close(listen_fd);
     return 0;
 }
