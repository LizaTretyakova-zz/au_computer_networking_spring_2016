#include "au_socket.h"

#include <cstdlib>
#include <iostream>
#include <string>

using std::cerr;
using std::endl;
using std::string;

void au_socket::check_socket_set() {
    cerr << "check socket with fd " << sockfd << endl;
    if (sockfd < 0) {
        throw std::runtime_error("Socket was not set properly, revert\n");
    }
}

void au_socket::get_sockaddr(hostname host_addr, au_stream_port port, struct sockaddr_in* dst) {
    int rv;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    cerr << "get_sockaddr" << endl;
    cerr << string(host_addr) << endl;

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

void au_socket::form_packet(const void* buf, size_t size) {
    // clear buffer
    memset(buffer, 0, AU_BUF_SIZE);

    // omit writing ip header!

    // write tcphdr
    struct tcphdr* tcph = (struct tcphdr*)(buffer);
    tcph->th_sport = local_port;
    tcph->th_dport = remote_port;
    int nwords = std::min(size, AU_BUF_CAPACITY);
    tcph->th_sum = checksum((unsigned short*)buf, nwords);

    // write data
    char* data = buffer + sizeof(tcphdr);
    memcpy(data, buf, nwords);
    cerr << "wrote data to buffer:\n" << string(data) << endl;
    buffer[AU_BUF_SIZE - 1] = 0;
}

bool au_socket::handshake(struct addrinfo* servinfo) {
    // the dummy one for now
    (void)servinfo;
    return true;
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

bool au_socket::control_csum(int nwords) {
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    unsigned short th_sum = tcph->th_sum;
    unsigned short csum = checksum(
                (unsigned short*)(buffer
                                  + sizeof(struct ip)
                                  + sizeof(struct tcphdr)),
                nwords);
    return th_sum == csum;
}

bool au_socket::is_syn() {
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    return tcph->th_flags == TH_SYN;
}

bool au_socket::is_ack() {
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    return tcph->th_flags == TH_ACK;
}

bool au_socket::is_syn_ack() {
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    return tcph->th_flags == (TH_SYN | TH_ACK);
}

bool au_socket::is_fin() {
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    return tcph->th_flags == TH_FIN;
}

bool au_socket::from(struct sockaddr_in* peer) {
    struct iphdr *iph = (struct iphdr *)buffer;
    return peer->sin_addr.s_addr == iph->saddr;
}

bool au_socket::is_ours() {
    struct iphdr *iph = (struct iphdr *)buffer;
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));

    cerr << "is_ours" << endl;

    cerr << "tcph->th_sport = " << tcph->th_sport << " tcph->th_dport = " << tcph->th_dport << endl;
    cerr << "remote_port    = " << remote_port << " local_port     = " << local_port << endl;
    if(tcph->th_sport != remote_port
            || tcph->th_dport != local_port) {
        return false;
    }

    struct sockaddr_in source;
    memset(&source, 0, sizeof(struct sockaddr_in));
    source.sin_addr.s_addr = iph->saddr;

    cerr << "source.sin_addr.s_addr = " << source.sin_addr.s_addr << endl;
    cerr << "remote_addr.sin_addr.s_addr = " << remote_addr.sin_addr.s_addr << endl;
    return source.sin_addr.s_addr == remote_addr.sin_addr.s_addr;
}
