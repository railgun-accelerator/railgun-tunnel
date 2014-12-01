#ifndef UTILS_H_
#define UTILS_H_

#include "common.h"

extern void payload_htonl(PAYLOAD_PACKET* ppayload);

extern void payload_ntohl(PAYLOAD_PACKET* ppayload);

extern int set_non_blocking(int sockfd);


#endif /*UTILS_H_*/
