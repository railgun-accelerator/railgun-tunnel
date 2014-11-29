#include <utils.h>

void payload_ntohl(PAYLOAD_PACKET* ppayload) {
    ppayload->seq = ntohl(ppayload->seq);
    ppayload->ack = ntohl(ppayload->seq);
}

void payload_htonl(PAYLOAD_PACKET* ppayload) {
    ppayload->seq = htonl(ppayload->seq);
    ppayload->ack = htonl(ppayload->seq);
}
