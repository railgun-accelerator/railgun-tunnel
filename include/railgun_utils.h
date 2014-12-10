/*
 * railgun_utils.h
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#ifndef RAILGUN_UTILS_H_
#define RAILGUN_UTILS_H_

#include <railgun_common.h>

extern int set_non_blocking(int sockfd);

extern int map_from_file(const char* filename, void ** pdata_buffer,
		u_int64_t *plength);

extern void unmap_from_file(int fd, void* data_buffer, int length);

static inline void payload_htonl(PAYLOAD_HEADER* ppayload) {
	ppayload->seq = htonl(ppayload->seq);
	ppayload->ack = htonl(ppayload->ack);
	int sc = ppayload->sack_cnt;
	if (sc != 0) {
		SACK_PACKET* sack = NULL;
		list_for_each_prev_entry(sack, &ppayload->sack_head, head) {
			sack->left_edge = htonl(sack->left_edge);
			sack->right_edge = htonl(sack->right_edge);
		}
	}
	ppayload->sack_cnt = htonl(ppayload->sack_cnt);
}

static inline void payload_ntohl(PAYLOAD_HEADER* ppayload) {
	ppayload->seq = ntohl(ppayload->seq);
	ppayload->ack = ntohl(ppayload->ack);
	int sc = ppayload->sack_cnt;
	if (sc != 0) {
		SACK_PACKET* sack = NULL;
		list_for_each_prev_entry(sack, &ppayload->sack_head, head) {
			sack->left_edge = ntohl(sack->left_edge);
			sack->right_edge = ntohl(sack->right_edge);
		}
	}
	ppayload->sack_cnt = ntohl(ppayload->sack_cnt);
}

static inline int before(u_int32_t seq1, u_int32_t seq2) {
	return (int32_t) (seq1 - seq2);
}

#define after(seq2, seq1) before(seq1, seq2);

static inline PAYLOAD_HEADER* railgun_payload_header(RAILGUN_HEADER* rp) {
	return (PAYLOAD_HEADER*) &rp->seq;
}

static inline PAYLOAD_HEADER* railgun_resp_header(RESP_HEADER* rp) {
	return (PAYLOAD_HEADER*) &rp->seq;
}

static inline u_int64_t get_current_time_in_millis() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline void print_readable_time(FILE* fd) {
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char buf[64];
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	size_t index = strftime(buf, (size_t)64, "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(&buf[index], (size_t)(64 - index), ".%06ld \n", tv.tv_usec);
	fprintf(fd, "%s", buf);
}
#endif /*RAILGUN_UTILS_H_*/
