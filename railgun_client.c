/*
 * railgun_client.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#include <sys/mman.h>
#include <sys/stat.h>

#include <railgun_common.h>
#include <railgun_utils.h>

#include <railgun_payload_queue.h>

RAILGUN_PACKET payload_head;
static int kdp_fd = 0, sock_fd = 0;

static int flag = 0;

static void retransmission_handler(int sig, siginfo_t *si, void *uc) {
	printf("retransmission triggered. \n");
	struct epoll_event* pev;
	pev = (struct epoll_event*) uc;
	pev->events = EPOLLIN | EPOLLOUT;
	int ret = epoll_ctl(kdp_fd, EPOLL_CTL_MOD, sock_fd, pev);
	if (ret < 0) {
		perror("epoll_ctl add output watch failed");
	}
}

int main(int argc, char** argv) {
	struct sockaddr_in servaddr;
	void *data_buffer, *iter_buf;
	int i = 0, nfds = 0, nbytes = 0, seq = 0, data_read_size = 0;
	int data_fd = 0;
	long filelength = 0;
	struct epoll_event ev;
	struct epoll_event events[MAXCLIENTEPOLLSIZE];
	char recvbuf[MAXBUF + 1];
	u_int64_t current_time_in_millis;

	/*check args*/
	if (argc != 3) {
		printf("usage: udpclient <addr> <data file>\n");
		exit(1);
	}

	/* init servaddr */
	bzero(recvbuf, MAXBUF + 1);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
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
	iter_buf = data_buffer = mmap(NULL, filelength, PROT_READ, MAP_PRIVATE,
			data_fd, 0);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (connect(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr))
			== -1) {
		perror("connect error");
		exit(1);
	}

	set_non_blocking(sock_fd);

	kdp_fd = epoll_create(MAXCLIENTEPOLLSIZE);
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = sock_fd;

	epoll_ctl(kdp_fd, EPOLL_CTL_ADD, sock_fd, &ev);
	bzero(&payload_head, sizeof(RAILGUN_PACKET));
	INIT_LIST_HEAD(&payload_head.head);
	railgun_timer_init(retransmission_handler, (void*) &ev);
	while (1) {
		nfds = epoll_wait(kdp_fd, events, 10000, -1);
		if (nfds < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("epoll_wait");
			goto error;
		}

		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLIN) {
				bzero(recvbuf, MAXBUF + 1);
				nbytes = read(sock_fd, recvbuf, MAXBUF);
				if (nbytes < 0) {
					if (errno != EAGAIN) {
						perror("read error");
					}
				} else {
					u_int32_t ack = ntohl(*(u_int32_t *) &recvbuf[0]);
					printf("recv %d from server \n", ack);
					RAILGUN_PACKET* packet = payload_queue_find(ack);
					if (packet != NULL) {
						printf("remove %d from table \n", ack);
						payload_queue_delete(packet);
					}
				}
			} else if (events[i].events & EPOLLOUT) {
				BOOL is_packet_send = FALSE;
				current_time_in_millis = get_current_time_in_millis();
				RAILGUN_PACKET *packet = NULL, *tmp = NULL;
				for_packet_in_payload_queue(packet, tmp)
				{
					if (flag) {
						printf(
								"iterate send queue current packet %d time = %lld, current_time_in_millis = %lld \n",
								packet->seq, packet->timestamp,
								current_time_in_millis);
					}
					if (packet->timestamp + (u_int64_t)RTO <= current_time_in_millis) {
						railgun_packet_write(packet, sock_fd);
						packet->timestamp = get_current_time_in_millis();
						is_packet_send = TRUE;
						break;
					}
				}
				if (!is_packet_send) {
					if (data_read_size + PAYLOAD_SIZE <= filelength) {
						int packet_skip_size = PAYLOAD_SIZE;
						RAILGUN_PACKET* pay_load = payload_queue_add(seq,
								iter_buf, packet_skip_size,
								get_current_time_in_millis());
						printf("add %d to table \n", seq);
						seq++;
						railgun_packet_write(pay_load, sock_fd);
						data_read_size += packet_skip_size;
						iter_buf += packet_skip_size;
					} else {
						if (is_payload_queue_empty) {
							printf("done. \n");
							railgun_timer_delete();
							goto error;
						} else {
							ev.events = EPOLLIN;
							int ret = epoll_ctl(kdp_fd, EPOLL_CTL_MOD, sock_fd,
									&ev);
							if (ret < 0) {
								perror("epoll_ctl remove output watch failed");
							}
							printf("set timer for %d \n",
									payload_queue_begin()->seq);
							flag = 1;
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
	error: munmap(data_buffer, filelength);
	close(data_fd);
	close(sock_fd);
	railgun_timer_delete();
	close(kdp_fd);
	exit(0);
}
