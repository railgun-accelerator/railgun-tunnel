/*
 * railgun_server.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#include <railgun_server.h>
#include <railgun_utils.h>

PAYOAD_RESP resp_head;

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

	if (bind(listen_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))
			== -1) {
		perror("bind failed!");
		exit(1);
	}

	bzero(&resp_head, sizeof(PAYOAD_RESP));
	INIT_LIST_HEAD(&resp_head.head);

	/*create epoll handle, add listen socket to epoll set*/
	kdp_fd = epoll_create(MAXSERVEREPOLLSIZE);
	ev.events = EPOLLIN;
	ev.data.fd = listen_fd;
	if (epoll_ctl(kdp_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
		fprintf(stderr, "epoll set input event listen error: fd = %d/n",
				listen_fd);
	}

	while (1) {
		nfds = epoll_wait(kdp_fd, events, 10000, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			break;
		}

		for (n = 0; n < nfds; ++n) {
			if ((events[n].events & EPOLLERR) || (events[n].events & EPOLLHUP)
					|| (!((events[n].events & EPOLLIN)
							|| events[n].events & EPOLLOUT))) {
				perror("epoll error\n");
				close(events[n].data.fd);
				continue;
			} else if (events[n].data.fd == listen_fd
					&& (events[n].events & EPOLLIN)) {
				struct sockaddr_in client_addr;
				socklen_t cli_len = sizeof(client_addr);
				ret = recvfrom(listen_fd, &packet, sizeof(PAYLOAD_PACKET), 0,
						(struct sockaddr*) &client_addr, &cli_len);
				//payload_ntohl(&packet);
				memcpy(sendbuf, &packet, sizeof(uint32_t));
				memcpy((sendbuf + 4), &packet, sizeof(uint32_t));
				if (ret > 0) {
					printf("socket %d recv from %s:%d message: %d bytes, ack = %d, \n",
							listen_fd, inet_ntoa(client_addr.sin_addr),
							ntohs(client_addr.sin_port), ret, ntohl(*(u_int32_t*)sendbuf));
				}

				ret = sendto(listen_fd, sendbuf, 2 * sizeof(uint32_t), 0,
						(struct sockaddr*) &client_addr, cli_len);
				if (ret < 0) {
					if (errno == EAGAIN) {
						PAYOAD_RESP* resp = resp_queue_add(*(u_int32_t*)sendbuf,  &client_addr);
						printf("send message pending: %u",
								ntohl(*(u_int32_t*) resp));
						ev.events = EPOLLOUT;
						if (epoll_ctl(kdp_fd, EPOLL_CTL_MOD, listen_fd, &ev)
								< 0) {
							fprintf(stderr,
									"epoll set output event listen error: fd = %d/n",
									listen_fd);
						}
					} else {
						printf("send message error: %s", strerror(errno));
					}
				} else {
					printf("socket %d send response %s:%d message: %d bytes, ack = %d, \n",
												listen_fd, inet_ntoa(client_addr.sin_addr),
												ntohs(client_addr.sin_port), ret, ntohl(*(u_int32_t*)sendbuf));
					fflush(stdout);
				}
			} else if (events[n].data.fd == listen_fd
					&& (events[n].events & EPOLLOUT)) {
				if (is_resp_queue_empty) {
					ev.events = EPOLLIN;
					if (epoll_ctl(kdp_fd, EPOLL_CTL_MOD, listen_fd, &ev) < 0) {
						fprintf(stderr,
								"epoll remove output event listen error: fd = %d/n",
								listen_fd);
					}
					continue;
				} else {
					PAYOAD_RESP* resp = resp_queue_begin();
					sendto(listen_fd, resp, 2 * sizeof(uint32_t), 0,
							(struct sockaddr*) &resp->addr, sizeof(resp->addr));
					resp_queue_delete(resp);
				}
			}
		}
	}

	close(listen_fd);
	return 0;
}
