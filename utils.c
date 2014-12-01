#include <common.h>
#include <utils.h>


void payload_ntohl(PAYLOAD_PACKET* ppayload) {
    ppayload->seq = ntohl(ppayload->seq);
    ppayload->ack = ntohl(ppayload->seq);
}

void payload_htonl(PAYLOAD_PACKET* ppayload) {
    ppayload->seq = htonl(ppayload->seq);
    ppayload->ack = htonl(ppayload->seq);
}

int set_non_blocking(int sockfd) {
    if (fcntl(sockfd, F_SETFL,
        fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}
