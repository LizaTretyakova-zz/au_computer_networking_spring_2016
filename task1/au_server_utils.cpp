#include "au_socket.h"

#include <cstdlib>
#include <iostream>
#include <string>

bool au_server_socket::to_this_server() {
    struct iphdr *iph = (struct iphdr *)buffer;
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));

    if(tcph->t.th_dport != local_port) {
        return false;
    }

    struct sockaddr_in source;
    memset(&source, 0, sizeof(struct sockaddr_in));
    source.sin_addr.s_addr = iph->daddr;

    return source.sin_addr.s_addr == local_addr.sin_addr.s_addr;
}
