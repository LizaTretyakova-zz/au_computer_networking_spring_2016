#include "au_socket.h"
#include "logging.h"

#include <cstdlib>
#include <iostream>
#include <string>

using std::cerr;
using std::endl;
using std::string;

void au_socket::check_socket_set() {
    // cerr << "check socket with fd " << sockfd << endl;
    if (sockfd < 0) {
        throw std::runtime_error("Socket was not set properly, revert\n");
    }
    if (state == DISCONNECTED) {
        throw std::runtime_error("Socket not connected, revert\n");
    }
}

void au_socket::get_sockaddr(hostname host_addr, au_stream_port port, struct sockaddr_in* dst) {
    int rv;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = 0;

    string service = std::to_string(port);

    if ((rv = getaddrinfo(host_addr, service.c_str(), &hints, &servinfo)) != 0) {
        throw std::runtime_error("getaddrinfo:" + std::string(gai_strerror(rv)) + "\n");
    }

    memcpy(dst, servinfo->ai_addr, sizeof(struct sockaddr));

    freeaddrinfo(servinfo);
}

unsigned short au_socket::checksum(unsigned short *buf, int nwords) {
    unsigned long sum = 0;
    for(; nwords > 0; --nwords) {
        sum += *buf++;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return ~sum;
}

bool au_socket::is_syn() {
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));
    return tcph->t.th_flags == TH_SYN;
}

bool au_socket::is_ack() {
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));
    return tcph->t.th_flags == TH_ACK;
}

bool au_socket::is_syn_ack() {
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));
    return tcph->t.th_flags == (TH_SYN | TH_ACK);
}

bool au_socket::is_fin() {
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));
    return tcph->t.th_flags == TH_FIN;
}

bool au_socket::from(struct sockaddr_in* peer) {
    struct iphdr *iph = (struct iphdr *)buffer;
    return peer->sin_addr.s_addr == iph->saddr;
}

bool au_socket::is_ours() {
    struct iphdr *iph = (struct iphdr *)buffer;
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));

    if(tcph->t.th_sport != remote_port
            || tcph->t.th_dport != local_port) {
        return false;
    }

    struct sockaddr_in source;
    memset(&source, 0, sizeof(struct sockaddr_in));
    source.sin_addr.s_addr = iph->saddr;

//    log("source.sin_addr.s_addr = " + std::to_string(source.sin_addr.s_addr)
//        + "\nremote_addr.sin_addr.s_addr = " + std::to_string(remote_addr.sin_addr.s_addr));
    return source.sin_addr.s_addr == remote_addr.sin_addr.s_addr;
}
