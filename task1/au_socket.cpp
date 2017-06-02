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

au_socket::au_socket(hostname a, au_stream_port cp, au_stream_port sp):
    addr(a), local_port(cp), remote_port(sp) {
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_AU)) == -1) {
        perror("AU client: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    cerr << "created socket with fd " << sockfd << endl;
    memset(buffer, 0, AU_BUF_SIZE);
}

au_socket::au_socket(int new_fd, hostname a, au_stream_port cp, au_stream_port sp):
    au_socket(a, cp, sp) {
    sockfd = new_fd;
}

//au_server_socket::au_server_socket(hostname a, au_stream_port port_number):
//    addr(a), port(port_number) {
//    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_AU)) == -1) {
//        perror("AU server: error creating socket");
//        throw std::runtime_error("socket() call failed");
//    }
//    memset(buffer, 0, AU_BUF_SIZE);
//}

void au_socket::send(const void* buf, size_t size) {
    check_socket_set();

    // Address resolution stuff
//    struct sockaddr_in sin;
//    memset (&sin, 0, sizeof (struct sockaddr_in));
//    get_remote_sockaddr(&sin);

    // Filling in the packet
    // (only one packet for now)
    /*
     * for i = 1 .. number_of_portions {
     *     set next header
     *     copy next portion of data form buf to internal buffer
     *     sendto(...)
     * }
     *
     * where number_of_portions == [size // buffer_capacity]
     */
    // TODO

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
    check_socket_set();

}

stream_socket* au_server_socket::accept_one_client() {
    check_socket_set();

    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    // catch a syn packet
    int res;
    do {
        res = recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size);
        if(res < 0) {
            perror("AU Socket: error while receiving");
            throw std::runtime_error("AU Socket: error while receiving");
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
    struct tcphdr response;
    memcpy(&response, tcph, sizeof(struct tcphdr));
    response.th_sport = tcph->th_dport;
    response.th_dport = tcph->th_sport;
    response.th_seq = htonl(rand());
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


    return new au_socket();
}
