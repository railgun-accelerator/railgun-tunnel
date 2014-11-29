#ifndef TYPE_DEF_H_
#define TYPE_DEF_H_

#define PAYLOAD_SIZE 1464

typedef struct payload_packet {
    u_int32_t seq;
    u_int32_t ack;
    u_int8_t data[PAYLOAD_SIZE];
} PAYLOAD_PACKET;


#endif /*TYPE_DEF_H_*/
