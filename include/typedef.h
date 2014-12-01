#ifndef TYPE_DEF_H_
#define TYPE_DEF_H_

#define PAYLOAD_SIZE 1464

typedef struct payload_packet {
    u_int32_t seq;
    u_int32_t ack;
    u_int8_t data[PAYLOAD_SIZE];
} PAYLOAD_PACKET;

typedef struct payload_resp {
    u_int32_t response1;
    u_int32_t response2;
    struct sockaddr_in addr;
} PAYOAD_RESP;
#endif /*TYPE_DEF_H_*/
