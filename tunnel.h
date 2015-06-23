//
// Created by root on 6/22/15.
//

#ifndef RAILGUN_TUNNEL_TUNNEL_H
#define RAILGUN_TUNNEL_TUNNEL_H

#define TICK 10000
#define MTU ETH_DATA_LEN-20-8-5

#include <stdio.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <Shorthair.hpp>

using namespace std;
using namespace cat;
using namespace shorthair;

class Tunnel : IShorthair {

    // Called with the latest data packet from remote host
    virtual void OnPacket(u8 *packet, int bytes);

    // Called with the latest OOB packet from remote host
    virtual void OnOOB(u8 *packet, int bytes);

    // Send raw data to remote host over UDP socket
    virtual void SendData(u8 *buffer, int bytes);

public:
    Shorthair codec;
    int socket_fd;
    int tun_fd;
    Tunnel(char* dev, char* local_addr, in_addr_t local_port, char* remote_addr, in_addr_t remote_port);
};


#endif //RAILGUN_TUNNEL_TUNNEL_H
