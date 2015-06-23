//
// Created by root on 6/22/15.
//

#include <iostream>
#include "tunnel.h"

Tunnel::Tunnel(char* dev, char* local_addr, in_addr_t local_port, char* remote_addr, in_addr_t remote_port){

    // init tun
    struct ifreq ifr;
    if( (tun_fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
        perror("error open tun");
        exit(1);
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    if( *dev )
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if(ioctl(tun_fd, TUNSETIFF, (void *) &ifr) < 0 ){
        close(tun_fd);
        perror("err set TUNSETIFF");
        exit(1);
    }

    // init socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_fd < 0){
        perror("error create socket");
        exit(1);
    }
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(local_port);
    if(inet_aton(local_addr,&addr.sin_addr)<0){
        perror("inet_aton local");
        exit(1);
    }
    bind(socket_fd,(struct sockaddr *)&addr,sizeof(addr));

    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remote_port);
    if(inet_aton(remote_addr,&addr.sin_addr)<0){
        perror("inet_aton remote");
        exit(1);
    }
    connect(socket_fd,(struct sockaddr *)&addr,sizeof(addr));

    // init codec

    Settings settings;
    settings.target_loss = 0.03;
    settings.max_delay = 100;
    settings.max_data_size = 1467;
    settings.interface = this;
    settings.conserve_bandwidth = true;

    codec.Initialize(settings);
}

void Tunnel::OnPacket(u8 *packet, int bytes){
    //cout << "tun write: " << bytes << endl;
    write(tun_fd, packet, bytes);
}
void Tunnel::OnOOB(u8 *packet, int bytes){
    cout << "oob" << endl;
}
void Tunnel::SendData(u8 *buffer, int bytes){
    //cout << "socket send: " << bytes << endl;
    send(socket_fd, buffer, bytes, 0);
}