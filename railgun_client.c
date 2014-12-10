/*
 * railgun_client.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#include <railgun_common.h>
#include <railgun_utils.h>

#include <railgun_payload_queue.h>

RAILGUN_HEADER payload_head;
static int g_kdp_fd = 0, g_sock_fd = 0;
static u_int32_t g_seq = ISN, g_ack = ISN;

static void retransmission_handler(int sig, siginfo_t *si, void *uc) {
	printf("retransmission triggered. \n");
	struct epoll_event* pev;
	pev = (struct epoll_event*) si->si_value.sival_ptr;
	pev->events = EPOLLIN | EPOLLOUT;
	int ret = epoll_ctl(g_kdp_fd, EPOLL_CTL_MOD, g_sock_fd, pev);
	if (ret < 0) {
		perror("epoll_ctl add output watch failed");
	}
}

int main(int argc, char** argv) {
	struct sockaddr_in servaddr;
	void *data_buffer;
	int i = 0, nfds = 0, data_read_size = 0;
	int data_fd = 0;
	u_int64_t filelength = 0;
	struct epoll_event ev;
	struct epoll_event events[MAXCLIENTEPOLLSIZE];
	u_int8_t recvbuf[MAXBUF];
	u_int8_t sendbuf[MAXBUF];
	u_int64_t current_time_in_millis;

	/*check args*/
	if (argc != 3) {
		printf("usage: udpclient <addr> <data file>\n");
		exit(1);
	}

	/* init servaddr */
	bzero(recvbuf, MAXBUF);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
		printf("[%s] is not a valid address\n", argv[1]);
		exit(1);
	}

	/* init data area*/
	data_fd = map_from_file(argv[2], &data_buffer, &filelength);
	g_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (connect(g_sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr))
			== -1) {
		perror("connect error");
		exit(1);
	}

	set_non_blocking(g_sock_fd);

	g_kdp_fd = epoll_create(MAXCLIENTEPOLLSIZE);
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = g_sock_fd;

	epoll_ctl(g_kdp_fd, EPOLL_CTL_ADD, g_sock_fd, &ev);
	bzero(&payload_head, sizeof(RAILGUN_HEADER));
	INIT_LIST_HEAD(&payload_head.head);
	railgun_timer_init(retransmission_handler, (void*) &ev);
	while (1) {
		nfds = epoll_wait(g_kdp_fd, events, 10000, -1);
		if (nfds < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("epoll_wait");
			goto error;
		}

		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLIN) {
				RESP_HEADER header;
				bzero(&header, sizeof(RESP_HEADER));
				railgun_resp_read(g_sock_fd, recvbuf, &header);
				printf("recv %d from server \n", header.seq);
				if (header.seq > g_ack) {
					g_ack = header.seq;
				}
				RAILGUN_HEADER *packet = NULL, *tmp = NULL;
				for_packet_in_payload_queue(packet, tmp)
				{
					if (packet->seq < g_ack) {
						payload_queue_delete(packet);
					} else {
						break;
					}
				}
				RAILGUN_HEADER * pos = packet;
				SACK_PACKET* psack = NULL, *sack_tmp = NULL;
				list_for_each_prev_entry(psack, &header.sack_head, head)
				{
					for_packet_in_payload_queue(packet, tmp)
					{
						if (packet != pos) {
							continue;
						}
						if (packet->seq < psack->left_edge) {
						} else if (packet->seq < psack->right_edge) {
							payload_queue_delete(packet);
						} else {
							break;
						}
					}
				}
				//release sack list in response.
				list_for_each_prev_entry_safe(psack, sack_tmp, &header.sack_head, head) {
					list_del(&psack->head);
					free(psack);
				}
			} else if (events[i].events & EPOLLOUT) {
				BOOL is_packet_send = FALSE;
				current_time_in_millis = get_current_time_in_millis();
				RAILGUN_HEADER *packet = NULL, *tmp = NULL;
				for_packet_in_payload_queue(packet, tmp)
				{
					if (packet->timestamp + (u_int64_t) RTO
							<= current_time_in_millis) {
						railgun_packet_write(packet, g_sock_fd, sendbuf,
								data_buffer, NULL);
						packet->timestamp = get_current_time_in_millis();
						is_packet_send = TRUE;
						break;
					}
				}
				if (!is_packet_send) {
					if (data_read_size + MTU <= filelength) {
						int packet_skip_size = 0;
						RAILGUN_HEADER* pay_load = payload_queue_add(g_seq,
								g_ack, data_read_size,
								get_current_time_in_millis());
						printf("add %d to table \n", pay_load->seq);
						railgun_packet_write(pay_load, g_sock_fd, sendbuf,
								data_buffer, &packet_skip_size);
						data_read_size += packet_skip_size;
						g_seq += packet_skip_size;
					} else {
						if (is_payload_queue_empty) {
							printf("done. \n");
							railgun_timer_delete();
							goto error;
						} else {
							ev.events = EPOLLIN;
							int ret = epoll_ctl(g_kdp_fd, EPOLL_CTL_MOD,
									g_sock_fd, &ev);
							if (ret < 0) {
								perror("epoll_ctl remove output watch failed");
							}
							printf("set timer for %d \n",
									payload_queue_begin()->seq);
							railgun_timer_set(
									RTO
											- (get_current_time_in_millis()
													- payload_queue_begin()->timestamp));
						}
					}
				}
			}
		}
	}
	error: unmap_from_file(data_fd, data_buffer, filelength);
	close(g_sock_fd);
	railgun_timer_delete();
	close(g_kdp_fd);
	exit(0);
}
