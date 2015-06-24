#include <pthread.h>
#include <errno.h>
#include <iostream>
#include <sys/time.h>
#include "tunnel.h"

static void *inbound_loop(void *tunnel_void){
    Tunnel *tunnel = (Tunnel *)tunnel_void;
    ssize_t ret;
    unsigned char buf[MTU];
    struct timeval last_tick, timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TICK;
    setsockopt(tunnel->socket_fd, SOL_SOCKET,SO_RCVTIMEO, &timeout, sizeof(timeout));
    while ( true ){
        ret = recv(tunnel->socket_fd, buf, sizeof(buf), 0);
        if (ret >= 0){
            //cout << "socket recv: " << ret << endl;
            tunnel->codec.Recv(buf, ret);

            gettimeofday(&timeout, NULL);
            timeout.tv_sec = last_tick.tv_sec - timeout.tv_sec;
            timeout.tv_usec = last_tick.tv_usec - timeout.tv_usec + TICK;
            if(timeout.tv_usec < 0){
                timeout.tv_sec -= 1;
                timeout.tv_usec += 1000000;
            }else if(timeout.tv_usec >= 1000000){
                timeout.tv_sec += 1;
                timeout.tv_usec -= 1000000;
            }
            setsockopt(tunnel->socket_fd, SOL_SOCKET,SO_RCVTIMEO, &timeout, sizeof(timeout));
            //cout << "sleep" << timeout.tv_sec << "." << timeout.tv_usec << endl;
        }else if(errno == EAGAIN) {
            //cout << "tick" << endl;
            tunnel->codec.Tick();
            gettimeofday(&last_tick, NULL);
            timeout.tv_usec = TICK;
            setsockopt(tunnel->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        }else if(errno == ECONNREFUSED){
            //do nothing
        }else{
            cout << errno << endl;
            perror("inbound loop recv");
            break;
        }
    }
}
static void *outbound_loop(void *tunnel_void){
    Tunnel *tunnel = (Tunnel *)tunnel_void;
    ssize_t ret;
    unsigned char buf[MTU];

    while ( (ret = read(tunnel->tun_fd, buf, sizeof(buf))) >= 0 ) {
        //cout << "tun read: " << ret << endl;
        tunnel->codec.Send(buf, ret);
    }
}
int main(int argc, char *argv[]) {

    if(argc != 6){
        cout << "Usage: " << argv[0] << " dev_name local_addr local_port remote_addr remote_port" << endl;
        exit(1);
    }

    Tunnel tunnel(argv[1], argv[2], atoi(argv[3]), argv[4], atoi(argv[5]));

    pthread_t thread;
    pthread_create(&thread, NULL, &outbound_loop, (void *) &tunnel);

    inbound_loop(&tunnel);

    return 0;
}