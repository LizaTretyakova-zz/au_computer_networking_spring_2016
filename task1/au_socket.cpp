#include "au_socket.h"
#include "stream_socket.h"
#include "pretty_print.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>            // strcpy, memset(), and memcpy()
#include <iostream>           // cerr
#include <netdb.h>            // struct addrinfo
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <stdexcept>
#include <string>
#include <sys/socket.h>       // needed for socket()
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t

using std::cerr;
using std::endl;
using std::string;

au_socket::au_socket(hostname a, au_stream_port cp, au_stream_port sp, state_t s):
    addr(a), local_port(cp), remote_port(sp), state(s) {
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_AU)) == -1) {
        perror("AU client: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    cerr << "created socket with fd " << sockfd
         << ", local_port " << cp
         << ", remote_port " << sp
         << " and state " << s << endl;
    memset(buffer, 0, AU_BUF_SIZE);
}

au_socket::au_socket(int new_fd, hostname a, au_stream_port cp, au_stream_port sp, state_t s):
    au_socket(a, cp, sp, s) {
    sockfd = new_fd;
}

void au_socket::send(const void* buf, size_t size) {
    check_socket_set();

    // form packet
    form_packet(buf, size);

    // send
    size_t data_len = sizeof(struct tcphdr) + strlen(buffer + sizeof(struct tcphdr));
    if(::sendto(sockfd, buffer, data_len, 0,
                (struct sockaddr *)(&remote_addr), sizeof(remote_addr)) < 0) {
        perror("AU Socket: error while sending");
        throw std::runtime_error("AU Socket: error while sending");
    }
}

void au_socket::recv(void *buf, size_t size) {
    check_socket_set();

    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    /*
     * while (hasn't yet received the finishing message) { */
    int res;
    do {
        res = recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size);
        if(res < 0) {
            perror("AU Socket: error while receiving");
            throw std::runtime_error("AU Socket: error while receiving");
        }
    } while(!is_ours());
    size_t iphdrlen = sizeof(struct ip);
    size_t tcphdrlen = sizeof(struct tcphdr);

    int nwords = (int)std::min((size_t)(res - iphdrlen - tcphdrlen), size);
    if(control_csum(nwords)) {
        memcpy(buf, buffer + iphdrlen + tcphdrlen, nwords);
        cerr << "received " << res << " bytes of data:" << endl;
    } else {
        cerr << "Checksum mismatch!" << endl;
    }
    /* }
     */
}

void au_client_socket::connect() {
    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    cerr << "Connecting" << endl;

    // send syn
    unsigned int c_seq = rand();
    struct tcphdr tcph;
    memset(&tcph, 0, sizeof(struct tcphdr));
    tcph.th_sport = local_port;
    tcph.th_dport = remote_port;
    tcph.th_seq = htonl(c_seq);
    tcph.th_off = 0; // sizeof(struct tcphdr) / 4;
    tcph.th_flags = TH_SYN;
    tcph.th_win = htonl(65535);
    tcph.th_sum = 0;

    if(::sendto(sockfd, &tcph, sizeof(struct tcphdr), 0,
                (struct sockaddr *)(&remote_addr), sizeof(remote_addr)) < 0) {
        perror("AU Client Socket: error while sending syn");
        throw std::runtime_error("AU Socket: error while sending");
    }

    // catch syn_ack
    do {
        if(recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size) < 0) {
            perror("AU Client Socket: error while receiving");
            throw std::runtime_error("AU Client Socket: error while receiving");
        }
    } while(!is_ours() || !from(&remote_addr) || !is_syn_ack());

    // check the correctness
    struct tcphdr* tcph_resp = (struct tcphdr*)(buffer + sizeof(struct ip));
    if(ntohl(tcph_resp->th_ack) != c_seq + 1) {
        cerr << "Error connecting to server: wrong seq" << endl;
        throw std::runtime_error("Wrong seq on client from server");
    }

    // happily acknowledge
    tcph.th_seq = htonl(ntohl(tcph_resp->th_seq) + 1);
    tcph.th_ack = 0;
    tcph.th_flags = TH_ACK;

    if(::sendto(sockfd, &tcph, sizeof(struct tcphdr), 0,
                (struct sockaddr*)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("failed to send ack");
        throw std::runtime_error("AU Client Socket: failed to send syn_ack");
    }

    state = CONNECTED;
    cerr << "Client: connected" << endl;
}

stream_socket* au_server_socket::accept_one_client() {
    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    cerr << "Accepting" << endl;

    // catch a syn packet
    do {
        if(recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size) < 0) {
            perror("AU Server Socket: error while receiving syn");
            throw std::runtime_error("AU Server Socket: error while receiving syn");
        }
    } while(!to_this_server() || !is_syn());

    // extract headers
    struct iphdr* iph = (struct iphdr*)buffer;
    struct tcphdr* tcph = (struct tcphdr*)(buffer + sizeof(struct ip));

    // remember sender's addr
    struct sockaddr_in client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    client.sin_addr.s_addr = iph->saddr;
    client.sin_family = AF_INET;
    client.sin_port = tcph->th_sport;

    // respond with a syn_ack packet
    unsigned int s_seq = rand();
    struct tcphdr response;
    memcpy(&response, tcph, sizeof(struct tcphdr));
    response.th_sport = tcph->th_dport;
    response.th_dport = tcph->th_sport;
    response.th_seq = htonl(s_seq);
    response.th_ack = htonl(ntohl(tcph->th_seq) + 1);
    response.th_off = 0; // sizeof(struct tcphdr) / 4;
    response.th_flags = TH_SYN | TH_ACK;
    response.th_win = htonl(65535);
    response.th_sum = 0;

    if(::sendto(sockfd, &response, sizeof(struct tcphdr), 0,
                (struct sockaddr*)&client, sizeof(client)) < 0) {
        perror("failed to send syn_ack");
        throw std::runtime_error("AU Server Socket: failed to send syn_ack");
    }

    // catch an ack packet
    do {
        if(recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size) < 0) {
            perror("AU Socket: error while receiving");
            throw std::runtime_error("AU Socket: error while receiving");
        }
    } while(!to_this_server() || !from(&client) || !is_ack());

    if(ntohl(tcph->th_seq) == s_seq + 1) {
        // return tuned socket
        char peer_addr[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &(client.sin_addr), peer_addr, INET_ADDRSTRLEN) == NULL) {
            perror("accept_one_client -- could not process client addr");
            throw std::runtime_error("AU Server Socket: inet_ntop");
        }
        au_stream_port new_port = rand();

        cerr << "Server: accepted" << endl;
        return new au_socket(peer_addr, new_port, client.sin_port, CONNECTED);
    }

    cerr << "Server: could not accept" << endl;
    return NULL;
}
