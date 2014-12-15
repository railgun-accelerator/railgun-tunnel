/*
 * railgun_client.c
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
	pev->events = EPOLLIN | EPOLLOUT;
	int ret = epoll_ctl(g_kdp_fd, EPOLL_CTL_MOD, g_udp_sock_fd, pev);
	if (ret < 0) {
		perror("epoll_ctl add output watch failed");
	}
}

int main(int argc, char** argv) {
	struct sockaddr_in listenaddr, conaddr;
	void *data_buffer;
	int i = 0, nfds = 0, data_read_size = 0;
	int data_fd = 0, tcp_server_fd = 0;
	u_int64_t filelength = 0;
	u_int16_t servport, conport;
	struct epoll_event udp_ev, tcp_ev;
	struct epoll_event events[MAXCLIENTEPOLLSIZE];
	struct timeval tv;
	u_int8_t recvbuf[MAXBUF];
	u_int8_t sendbuf[MAXBUF];
	u_int64_t current_time_in_millis;
	RAILGUN_HEADER railgun_header;

	/*check args*/
	if (argc != 5) {
		printf(
				"usage: railgun_client <listen port> <connect addr> <connect port> \n");
		exit(1);
	}

	/* init addr */
	bzero(recvbuf, MAXBUF);
	bzero(&listenaddr, sizeof(listenaddr));
	bzero(&conaddr, sizeof(conaddr));
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(atoi(argv[1]));
	listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	conaddr.sin_family = AF_INET;
	conaddr.sin_port = htons(atoi(argv[3]));
	if (inet_pton(AF_INET, argv[2], &conaddr.sin_addr) <= 0) {
		printf("[%s] is not a valid address\n", argv[1]);
		exit(1);
	}

//	data_fd = map_from_file(argv[2], &data_buffer, &filelength);

	/* init tcp server sock*/
	tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(tcp_server_fd, (struct sockaddr *) &listenaddr, sizeof(listenaddr))
			< 0) {
		perror("bind error");
		exit(1);
	}
	listen(tcp_server_fd, 1);
	g_tcp_sock_fd = accept(tcp_server_fd, (struct sockaddr*) NULL, NULL);

	/* init udp client sock*/
	g_udp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (connect(g_udp_sock_fd, (struct sockaddr *) &conaddr, sizeof(conaddr))
			== -1) {
		perror("connect error");
		exit(1);
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
			if (events[n].data.fd == g_tcp_sock_fd
					&& (events[i].events & EPOLLIN)) {
				//tcp read
				u_int32_t read_size = g_sd_buf_offset + BUFFER - g_ready;
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
			} else if (events[n].data.fd == g_tcp_sock_fd
					&& (events[i].events & EPOLLOUT)) {
				//tcp write
				u_int32_t write_size = g_ack - g_rcv_buf_offset;
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
			} else if (events[n].data.fd == g_udp_sock_fd
					&& (events[i].events & EPOLLIN)) {
				u_int32_t payload_offset = 0;
				u_int32_t read_cnt = 0;
				//udp read
				read_cnt = railgun_udp_read(g_udp_sock_fd, recvbuf,
						&railgun_header, &payload_offset);
				if (railgun_header.ack + railgun_header.win > g_window) {
					window = railgun_header.ack + railgun_header.win;
				}
				if (railgun_header.ack == g_window) {
					g_zero_win_probe_time = get_current_time_in_millis(*tv);
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
						epoll_add_watch(g_kdp_fd, g_tdp_sock_fd, &tcp_ev,
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
								recvbuf[payload_offset],
								read_cnt - payload_offset);
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
			} else if (events[n].data.fd == g_udp_sock_fd
					&& (events[i].events & EPOLLOUT)) {
				//udp write
				BOOL is_packet_send = FALSE;
				current_time_in_millis = get_current_time_in_millis(&tv);
				RAILGUN_HEADER *packet = NULL;
				for_packet_in_payload_queue(packet)
				{
					if (packet->timestamp + (u_int64_t) RTO
							<= current_time_in_millis) {
						SACK_PACKET *sack_iterp, sack_p;
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
							memcpy(&sack_p[i], iter_sack, sizeof(SACK_PACKET));
							_list_add(&(&sack_p[i++])->head,
									&packet->sack_head);
						}
						railgun_udp_write(packet, g_udp_sock_fd, sendbuf,
								g_send_buffer);
						packet->timestamp = get_current_time_in_millis(&tv);
						is_packet_send = TRUE;
						break;
					}
				}
				if (!is_packet_send) {
					if (data_read_size + MTU <= filelength) {
						int packet_skip_size = 0;
						RAILGUN_HEADER* pay_load = payload_queue_add(g_seq,
								g_ack, data_read_size,
								get_current_time_in_millis(&tv));
						printf("add %d to table \n", pay_load->seq);
						railgun_packet_write(pay_load, g_udp_sock_fd, sendbuf,
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
									g_udp_sock_fd, &ev);
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
	error:
//	unmap_from_file(data_fd, data_buffer, filelength);
	close(g_udp_sock_fd);
	close(g_tcp_sock_fd);
	close(tcp_server_fd);
	railgun_timer_delete();
	close(g_kdp_fd);
	exit(0);
}
