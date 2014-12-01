#include <udp_server.h>
#include <utils.h>

static PAYOAD_RESP* resp_pending_list[1024] = {0};
static int resp_pending_size = 0;
 
 int main(int argc, char** argv) {
     int listen_fd, kdp_fd, nfds, n, ret;
     struct sockaddr_in my_addr;
     struct epoll_event ev;
     struct epoll_event events[MAXSERVEREPOLLSIZE];
     struct rlimit rt;
     char sendbuf[MAXBUF + 1];
     PAYLOAD_PACKET packet;
     
     /*set max opend fd*/
     rt.rlim_max = rt.rlim_cur = MAXSERVEREPOLLSIZE;
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
     bzero(&sendbuf, MAXBUF + 1);
     bzero(&my_addr, sizeof(struct sockaddr_in));
     my_addr.sin_family = PF_INET;
     my_addr.sin_port = htons(SERV_PORT);
     my_addr.sin_addr.s_addr = INADDR_ANY;
     
     if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
         perror("bind failed!");
         exit(1);
     }
     
     /*create epoll handle, add listen socket to epoll set*/
     kdp_fd = epoll_create(MAXSERVEREPOLLSIZE);
     ev.events = EPOLLIN;
     ev.data.fd = listen_fd;
     if (epoll_ctl(kdp_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
         fprintf(stderr, "epoll set input event listen error: fd = %d/n", listen_fd);
     }
     
     while(1) {
         nfds = epoll_wait(kdp_fd, events, 10000, -1);
         if (nfds == -1) {
             perror("epoll_wait");
             break;
         }
         
         for(n = 0; n < nfds; ++n) {
             if ((events[n].events & EPOLLERR) || (events[n].events & EPOLLHUP)
              || (!(events[n].events & EPOLLIN || events[n].events & EPOLLOUT))) {
                 perror("epoll error\n");
                 close(events[n].data.fd);
	             continue;
             } else if(events[n].data.fd == listen_fd && (events[n].events & EPOLLIN)) {
                 struct sockaddr_in client_addr;
                 socklen_t cli_len = sizeof(client_addr);
                 ret = recvfrom(listen_fd, &packet, sizeof(PAYLOAD_PACKET), 0, (struct sockaddr*)&client_addr, &cli_len);
                 if (ret > 0) {
                    printf("socket %d recv from %s:%d message: %d bytes\n",
                    listen_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ret);
                 }
                 //payload_ntohl(&packet);
                 memcpy(sendbuf, &packet, sizeof(uint32_t));
                 memcpy((sendbuf + 4), &packet, sizeof(uint32_t));
                 
                 ret = sendto(listen_fd, sendbuf, 2 * sizeof(uint32_t), 0, (struct sockaddr*)&client_addr, cli_len);
                 if (ret < 0) {
                     if (errno == EAGAIN) {
                         PAYOAD_RESP* resp = (PAYOAD_RESP*)malloc(sizeof(PAYOAD_RESP));
                         resp->response1 = (u_int32_t)*sendbuf;
                         resp->response2 = (u_int32_t)*(sendbuf + 4);
                         memcpy(&resp->addr, &client_addr, cli_len);
                         resp_pending_list[resp_pending_size++] = resp;
                         printf("send message pending: %u", ntohl(*(u_int32_t*)resp));
                         ev.events = EPOLLOUT;
                         if (epoll_ctl(kdp_fd, EPOLL_CTL_MOD, listen_fd, &ev) < 0) {
                             fprintf(stderr, "epoll set output event listen error: fd = %d/n", listen_fd);
                         }
                     } else {
                         printf("send message error: %s", strerror(errno));
                     }
                 } else {
                     fflush(stdout);
                 }
             } else if(events[n].data.fd == listen_fd && (events[n].events & EPOLLOUT)) {
                 if (resp_pending_list == 0) {
                     ev.events = EPOLLIN;
                     if (epoll_ctl(kdp_fd, EPOLL_CTL_MOD, listen_fd, &ev) < 0) {
                         fprintf(stderr, "epoll remove output event listen error: fd = %d/n", listen_fd);
                     }
                     continue;
                 } else {
                     PAYOAD_RESP* resp = resp_pending_list[resp_pending_size];
                     resp_pending_list[resp_pending_size--] = 0;
                     sendto(listen_fd, resp, 2 * sizeof(uint32_t), 0, (struct sockaddr*)&resp->addr, sizeof(resp->addr));
                     free(resp);
                 }
             }
         }
     }
     
     close(listen_fd);
     return 0;
 }
