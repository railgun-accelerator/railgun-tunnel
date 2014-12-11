/*
 * railgun_server.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#include <railgun_server.h>
#include <railgun_utils.h>
#include <railgun_sack.h>

RESP_HEADER resp_head;
SACK_PACKET sack_head;

static u_int32_t g_seq = ISN, g_ack = ISN;
static BOOL g_timer_counter = FALSE;

static int g_listen_fd = 0, g_kdp_fd = 0;

typedef struct ato_event {
	struct epoll_event* pev;
	struct sockaddr_in addr;
	size_t addr_len;
} ATO_EVENT;

static void ato_handler(int sig, siginfo_t *si, void *uc) {
	printf("ato triggered. \n");
	ATO_EVENT* pato_ev;
	pato_ev = (ATO_EVENT*) si->si_value.sival_ptr;
	resp_queue_add_allocate(g_ack, g_seq, &pato_ev->addr, pato_ev->addr_len,
			sack_queue_size(), &sack_head);
	pato_ev->pev->events = EPOLLIN | EPOLLOUT;
	int ret = epoll_ctl(g_kdp_fd, EPOLL_CTL_MOD, g_listen_fd, pato_ev->pev);
	if (ret < 0) {
		perror("epoll_ctl add output watch failed");
	}
	free(pato_ev);
	g_timer_counter = FALSE;
}

int main(int argc, char** argv) {
	int nfds, n;
	struct sockaddr_in my_addr;
	struct epoll_event ev;
	struct epoll_event events[MAXSERVEREPOLLSIZE];
	struct rlimit rt;
	u_int8_t sendbuf[MAXBUF];
	u_int8_t recvbuf[MAXBUF];
	RESP_HEADER header;

	/*set max opend fd*/
	rt.rlim_max = rt.rlim_cur = MAXSERVEREPOLLSIZE;
	if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
		perror("setrilimit");
		exit(1);
	}

	/*start listen*/
	if ((g_listen_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket create failed !");
		exit(1);
	}

	/*set socket opt, port reusable*/
	int opt = SO_REUSEADDR;
	setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	set_non_blocking(g_listen_fd);
	bzero(sendbuf, MAXBUF);
	bzero(recvbuf, MAXBUF);
	bzero(&my_addr, sizeof(struct sockaddr_in));
	my_addr.sin_family = PF_INET;
	my_addr.sin_port = htons(SERV_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(g_listen_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))
			== -1) {
		perror("bind failed!");
		exit(1);
	}

	bzero(&resp_head, sizeof(RESP_HEADER));
	bzero(&sack_head, sizeof(SACK_PACKET));
	INIT_LIST_HEAD(&resp_head.head);
	INIT_LIST_HEAD(&sack_head.head);

	/*create epoll handle, add listen socket to epoll set*/
	g_kdp_fd = epoll_create(MAXSERVEREPOLLSIZE);
	ev.events = EPOLLIN;
	ev.data.fd = g_listen_fd;
	if (epoll_ctl(g_kdp_fd, EPOLL_CTL_ADD, g_listen_fd, &ev) < 0) {
		fprintf(stderr, "epoll set input event listen error: fd = %d/n",
				g_listen_fd);
	}

	while (1) {
		nfds = epoll_wait(g_kdp_fd, events, 10000, -1);
		if (nfds == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("epoll_wait");
			break;
		}

		for (n = 0; n < nfds; ++n) {
			if ((events[n].events & EPOLLERR) || (events[n].events & EPOLLHUP)
					|| (!((events[n].events & EPOLLIN)
							|| (events[n].events & EPOLLOUT)))) {
				perror("epoll error\n");
				close(events[n].data.fd);
				continue;
			} else if (events[n].data.fd == g_listen_fd
					&& (events[n].events & EPOLLIN)) {
				int payload_offset;
				bzero(&header, sizeof(RESP_HEADER));
				header.addr_len = sizeof(header.addr);
				int read_size = railgun_packet_read(g_listen_fd, recvbuf,
						&header);
				//test if we need to allocate memory for sack in payload first.
				int seq = ntohl(*(u_int32_t*) recvbuf);
				if (seq >= g_ack) {
					if (seq == g_ack) {
						//do not allocate memory for sack list.
						railgun_resp_allocate(recvbuf, &header, &payload_offset,
								0);
						g_ack += (read_size - payload_offset);
						if (!is_sack_queue_empty) {
							SACK_PACKET* psack = sack_queue_begin();
							if (psack->left_edge == g_ack) {
								g_ack = psack->right_edge;
								sack_queue_delete(psack);
							}
						}
					} else {
						//do allocate memory for sack list.
						railgun_resp_allocate(recvbuf, &header, &payload_offset,
								1);
						sack_queue_combine(header.seq,
								header.seq + read_size - payload_offset);
					}
				}
				if (g_timer_counter == FALSE) {
					ATO_EVENT *pevent = (ATO_EVENT *) malloc(sizeof(ATO_EVENT));
					bzero(pevent, sizeof(ATO_EVENT));
					pevent->addr_len = header.addr_len;
					pevent->pev = &ev;
					memcpy(&pevent->addr, &header.addr, pevent->addr_len);
					railgun_timer_init(ato_handler, (void*) pevent);
					railgun_timer_set(ATO);
					g_timer_counter = TRUE;
				}
			} else if (events[n].data.fd == g_listen_fd
					&& (events[n].events & EPOLLOUT)) {
				if (is_resp_queue_empty) {
					ev.events = EPOLLIN;
					if (epoll_ctl(g_kdp_fd, EPOLL_CTL_MOD, g_listen_fd, &ev)
							< 0) {
						fprintf(stderr,
								"epoll remove output event listen error: fd = %d/n",
								g_listen_fd);
					}
					continue;
				} else {
					printf("send response! \n");
					RESP_HEADER* resp = resp_queue_begin();
					railgun_resp_send(resp, g_listen_fd, sendbuf);
					resp_queue_delete(resp);
				}
			}
		}
	}

	close(g_listen_fd);
	close(g_kdp_fd);
	exit(0);
}
