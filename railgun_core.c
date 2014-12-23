/*
 * railgun_core.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#include <railgun_common.h>
#include <railgun_utils.h>

#include <railgun_sack.h>

#include <railgun_payload_queue.h>

RAILGUN_HEADER payload_head;
SACK_PACKET sack_head;

static int g_kdp_fd = 0, g_udp_sock_fd = 0, g_tcp_sock_fd = 0;
static u_int32_t g_seq = ISN, g_ack = ISN, g_window = ISN + BUFFER, g_ready =
ISN, g_sd_buf_offset = ISN, g_rcv_buf_offset = ISN;
static u_int8_t g_send_buffer[BUFFER] = { 0 }, g_recv_buffer[BUFFER] = { 0 };
static BOOL g_timer_counter = FALSE;
static u_int64_t g_delay_ack_time = 0, g_zero_win_probe_time = 0;
static u_int8_t g_zero_win_probe_content = 0;

static void retransmission_handler(int sig, siginfo_t *si, void *uc) {
	printf("retransmission triggered. \n");
	struct epoll_event* pev;
	pev = (struct epoll_event*) si->si_value.sival_ptr;
	epoll_add_watch(g_kdp_fd, g_udp_sock_fd, pev, EPOLLOUT);
}

int main(int argc, char** argv) {
	struct sockaddr_in listenaddr, conaddr;
	int i = 0, nfds = 0, udp_write_size = 0;
	int tcp_fd = 0;
	struct epoll_event udp_ev, tcp_ev;
	struct epoll_event events[MAXCLIENTEPOLLSIZE];
	struct timeval tv;
	u_int8_t recvbuf[MAXBUF];
	u_int8_t sendbuf[MAXBUF];
	u_int64_t current_time_in_millis;
	RAILGUN_HEADER railgun_header;
	BOOL is_client;

	/*check args*/
	if (argc != 6) {
		printf(
				"usage: railgun_core [server|client] <listen_addr> <listen port> <connect addr> <connect port> \n");
		exit(1);
	}
	if (strcmp(argv[2], "server") && strcmp(argv[2], "client")) {
		printf(
				"usage: railgun_core [server|client] <listen_addr> <listen port> <connect addr> <connect port> \n");
		exit(1);
	} else {
		is_client = !strcmp(argv[2], "client") ? TRUE : FALSE;
	}

	/* init addr */
	bzero(recvbuf, MAXBUF);
	bzero(&listenaddr, sizeof(listenaddr));
	bzero(&conaddr, sizeof(conaddr));
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(atoi(argv[3]));
	if (inet_pton(AF_INET, argv[2], &listenaddr.sin_addr) <= 0) {
		printf("[%s] is not a valid address\n", argv[1]);
		exit(1);
	}

	conaddr.sin_family = AF_INET;
	conaddr.sin_port = htons(atoi(argv[5]));
	if (is_client) {
		if (inet_pton(AF_INET, argv[4], &conaddr.sin_addr) <= 0) {
			printf("[%s] is not a valid address\n", argv[1]);
			exit(1);
		}
	} else {
		conaddr.sin_addr.s_addr = INADDR_ANY;
	}

	/* create udp sock*/
	g_udp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	/* create tcp sock*/
	tcp_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (is_client) {
		if (bind(tcp_fd, (struct sockaddr *) &listenaddr, sizeof(listenaddr))
				< 0) {
			perror("bind error");
			exit(1);
		}
		listen(tcp_fd, 1);
		g_tcp_sock_fd = accept(tcp_fd, (struct sockaddr*) NULL, NULL);
	} else {
		g_tcp_sock_fd = connect(tcp_fd, (struct sockaddr *) &listenaddr,
				sizeof(listenaddr));
		printf("tcp connect succeed! \n");
	}

	if (is_client) {
		if (connect(g_udp_sock_fd, (struct sockaddr *) &conaddr,
				sizeof(conaddr)) == -1) {
			perror("connect error");
			exit(1);
		}
	} else {
		if (bind(g_udp_sock_fd, (struct sockaddr *) &conaddr,
				sizeof(struct sockaddr)) == -1) {
			perror("bind failed!");
			exit(1);
		}
	}

	set_non_blocking(g_udp_sock_fd);
	set_non_blocking(g_tcp_sock_fd);

	g_kdp_fd = epoll_create(MAXCLIENTEPOLLSIZE);

	//add udp client sock's in&out to epoll watch list.
	epoll_init_watch(g_kdp_fd, g_udp_sock_fd, &udp_ev, EPOLLIN | EPOLLOUT);

	//add tcp client sock's input to epoll watch list.
	epoll_init_watch(g_kdp_fd, g_tcp_sock_fd, &tcp_ev, EPOLLIN);

	bzero(&payload_head, sizeof(RAILGUN_HEADER));
	INIT_LIST_HEAD(&payload_head.head);

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
			if (events[i].data.fd == g_tcp_sock_fd
					&& (events[i].events & EPOLLIN)) {
				//tcp read
				int read_size = g_sd_buf_offset + BUFFER - g_ready;
				//FIXME: send buffer can be handled more elegantly.
				railgun_tcp_read(g_tcp_sock_fd, g_send_buffer,
						g_ready - g_sd_buf_offset, &read_size);
				g_ready += read_size;
				if (g_ready == g_sd_buf_offset + BUFFER) {
					epoll_remove_watch(g_kdp_fd, g_tcp_sock_fd, &tcp_ev,
					EPOLLIN);
				}
				if (g_timer_counter == TRUE) {
					railgun_timer_delete();
					g_timer_counter = FALSE;
					epoll_add_watch(g_kdp_fd, g_udp_sock_fd, &udp_ev, EPOLLOUT);
				}
			} else if (events[i].data.fd == g_tcp_sock_fd
					&& (events[i].events & EPOLLOUT)) {
				//tcp write
				int write_size = g_ack - g_rcv_buf_offset;
				railgun_tcp_write(g_tcp_sock_fd, g_recv_buffer,
						g_rcv_buf_offset, &write_size);
				g_rcv_buf_offset += write_size;
				if (g_rcv_buf_offset == g_ack) {
					epoll_remove_watch(g_kdp_fd, g_tcp_sock_fd, &tcp_ev,
					EPOLLOUT);
				}
				if (g_delay_ack_time == 0) {
					g_delay_ack_time = get_current_time_in_millis(&tv);
				}
				if (g_timer_counter == TRUE) {
					railgun_timer_delete();
					g_timer_counter = FALSE;
					epoll_add_watch(g_kdp_fd, g_udp_sock_fd, &udp_ev, EPOLLOUT);
				}
			} else if (events[i].data.fd == g_udp_sock_fd
					&& (events[i].events & EPOLLIN)) {
				int payload_offset = 0;
				int read_cnt = 0;
				//udp read
				read_cnt = railgun_udp_read(g_udp_sock_fd, recvbuf,
						&railgun_header, &payload_offset);
				if (railgun_header.ack + railgun_header.win > g_window) {
					g_window = railgun_header.ack + railgun_header.win;
				}
				if (railgun_header.ack == g_window) {
					g_zero_win_probe_time = get_current_time_in_millis(&tv);
					if (railgun_header.ack > g_sd_buf_offset) {
						g_zero_win_probe_content =
								g_send_buffer[railgun_header.ack
										- g_sd_buf_offset - 1];
					}
					if (g_timer_counter == TRUE) {
						railgun_timer_delete();
						g_timer_counter = FALSE;
						epoll_add_watch(g_kdp_fd, g_udp_sock_fd, &udp_ev,
						EPOLLOUT);
					}
				} else {
					g_zero_win_probe_time = 0;
				}
				if (railgun_header.seq > g_ack) {
					g_ack = railgun_header.seq;
				}
				RAILGUN_HEADER *packet = NULL, *tmp = NULL;
				for_packet_in_payload_queue_safe(packet, tmp)
				{
					if (packet->seq < g_ack) {
						payload_queue_delete(packet);
					} else {
						break;
					}
				}
				RAILGUN_HEADER * pos = packet;
				SACK_PACKET* psack = NULL,
				*sack_tmp = NULL;
				list_for_each_prev_entry(psack, &railgun_header.sack_head, head)
				{
					for_packet_in_payload_queue_safe(packet, tmp)
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
				list_for_each_prev_entry_safe(psack, sack_tmp, &railgun_header.sack_head, head)
				{
					list_del(&psack->head);
					free(psack);
				}
				if (railgun_header.ack > g_sd_buf_offset) {
					//FIXME: we need to copy any content?
					if (g_sd_buf_offset + BUFFER == g_ready) {
						epoll_add_watch(g_kdp_fd, g_tcp_sock_fd, &tcp_ev,
						EPOLLIN);
					}
					g_sd_buf_offset = railgun_header.ack;
				}
				//if we have payload.
				if (read_cnt > payload_offset) {
					if (railgun_header.seq >= g_rcv_buf_offset) {
						memcpy(
								g_recv_buffer + railgun_header.seq
										- g_rcv_buf_offset,
								&recvbuf[payload_offset],
								(size_t) (read_cnt - payload_offset));
						if (railgun_header.seq >= g_ack) {
							if (railgun_header.seq == g_ack) {
								if (g_rcv_buf_offset == g_ack) {
									epoll_add_watch(g_kdp_fd, g_tcp_sock_fd,
											&tcp_ev, EPOLLOUT);
								}
								g_ack += (read_cnt - payload_offset);
								if (!is_sack_queue_empty) {
									SACK_PACKET* psack = sack_queue_begin();
									if (psack->left_edge == g_ack) {
										g_ack = psack->right_edge;
										sack_queue_delete(psack);
									}
								}
							} else {
								sack_queue_combine(railgun_header.seq,
										railgun_header.seq + read_cnt
												- payload_offset);
							}
						}
					}
					if (g_delay_ack_time == 0) {
						g_delay_ack_time = get_current_time_in_millis(&tv);
					}
					if (g_timer_counter == TRUE) {
						railgun_timer_delete();
						g_timer_counter = FALSE;
						epoll_add_watch(g_kdp_fd, g_udp_sock_fd, &udp_ev,
						EPOLLOUT);
					}
				}
			} else if (events[i].data.fd == g_udp_sock_fd
					&& (events[i].events & EPOLLOUT)) {
				//udp write
				BOOL is_packet_send = FALSE;
				current_time_in_millis = get_current_time_in_millis(&tv);
				RAILGUN_HEADER *packet = NULL;
				for_packet_in_payload_queue(packet)
				{
					if (packet->timestamp + (u_int64_t) RTO
							<= current_time_in_millis) {
						SACK_PACKET *sack_iterp, *sack_p;
						packet->ack = g_ack;
						packet->win = g_rcv_buf_offset + BUFFER - g_ack;
						int cur_sack_size = sack_queue_size();
						if (cur_sack_size < packet->sack_cnt) {
							packet->sack_cnt = cur_sack_size;
						}
						cur_sack_size = packet->sack_cnt;
						if (cur_sack_size != 0) {
							sack_p = (SACK_PACKET*) calloc(cur_sack_size,
									sizeof(SACK_PACKET));
						}
						int i = 0;
						for_sack_in_queue(sack_iterp)
						{
							if (i >= cur_sack_size) {
								break;
							}
							memcpy(&sack_p[i], sack_iterp, sizeof(SACK_PACKET));
							_list_add(&(&sack_p[i++])->head,
									&packet->sack_head);
						}
						railgun_udp_write(packet, g_udp_sock_fd, sendbuf,
								g_send_buffer);
						is_packet_send = TRUE;
						packet->timestamp = get_current_time_in_millis(&tv);
						payload_queue_move_tail(packet);
						g_zero_win_probe_time = 0;
						break;
					}
				}
				if (!is_packet_send) {
					//send data
					if (min(g_window, g_ready) - g_seq > 0) {
						RAILGUN_HEADER* payload = payload_queue_add_allocate(
								g_seq, g_ack, g_rcv_buf_offset + BUFFER - g_ack,
								g_seq, get_current_time_in_millis(&tv),
								sack_queue_size(), &sack_head);
						printf("add %d to table \n", payload->seq);
						udp_write_size = railgun_udp_write(payload,
								g_udp_sock_fd, sendbuf, g_send_buffer);
						is_packet_send = TRUE;
						//FIXME: what to do if we write failed.
						g_seq += (
								udp_write_size > payload->railgun_data_length ?
										payload->railgun_data_length : 0);
					}
				}
				if (!is_packet_send) {
					//zero window probe
					if (g_zero_win_probe_time + RTO
							<= get_current_time_in_millis(&tv)) {
						RAILGUN_HEADER* zero_win_probe = zero_probe_allocate(
								g_sd_buf_offset, g_ack,
								g_rcv_buf_offset + BUFFER - g_ack,
								get_current_time_in_millis(&tv),
								sack_queue_size(), &sack_head, 1,
								&g_zero_win_probe_content);
						railgun_udp_write(zero_win_probe, g_udp_sock_fd,
								sendbuf, NULL);
						is_packet_send = TRUE;
						g_zero_win_probe_time = get_current_time_in_millis(&tv);
						railgun_header_release(zero_win_probe);
					}
				}
				if (!is_packet_send) {
					if (g_delay_ack_time + ATO
							<= get_current_time_in_millis(&tv)) {
						RAILGUN_HEADER* payload = payload_queue_allocate(g_seq,
								g_ack, g_rcv_buf_offset + BUFFER - g_ack, 2,
								NULL, get_current_time_in_millis(&tv),
								sack_queue_size(), &sack_head);
						railgun_udp_write(payload, g_udp_sock_fd, sendbuf,
						NULL);
						is_packet_send = TRUE;
						railgun_header_release(payload);
					}
				}
				if (is_packet_send) {
					g_delay_ack_time = 0;
				} else {
					epoll_remove_watch(g_kdp_fd, g_udp_sock_fd, &udp_ev,
					EPOLLOUT);
					int time = min(
							g_zero_win_probe_time == 0 ?
									(u_int32_t) (-1) :
									g_zero_win_probe_time + RTO,
							is_payload_queue_empty ?
									(u_int32_t) (-1) :
									payload_queue_begin()->timestamp + RTO);
					time = min(time,
							g_delay_ack_time == 0 ?
									(u_int32_t) (-1) : g_delay_ack_time + ATO);
					if (time != (u_int32_t) (-1)) {
						railgun_timer_init(retransmission_handler,
								(void*) &udp_ev);
						railgun_timer_set(time);
					}
				}
			}
		}
	}
	error:
//	unmap_from_file(data_fd, data_buffer, filelength);
	close(g_udp_sock_fd);
	close(g_tcp_sock_fd);
	close(tcp_fd);
	railgun_timer_delete();
	close(g_kdp_fd);
	exit(0);
}
