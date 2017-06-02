#include "au_socket.h"

#include <cstdlib>
#include <iostream>
#include <string>

void au_client_socket::send_syn() {
    struct tcphdr* tcph = (struct tcphdr*)(buffer);

    tcph->th_sport = local_port;
    tcph->th_dport = remote_port;
    tcph->th_seq = htonl(rand());
    tcph->th_off = 0; // sizeof(struct tcphdr) / 4;
    tcph->th_flags = TH_SYN;
    tcph->th_win = htonl(65535);
    tcph->th_sum = 0;

    // send
    if(::sendto(sockfd, buffer, sizeof(struct tcphdr), 0,
                (struct sockaddr *)(&remote_addr), sizeof(remote_addr))) {
        perror("AU Socket: error while sending syn");
        throw std::runtime_error("AU Socket: error while sending");
    }
}
