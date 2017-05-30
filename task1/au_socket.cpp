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

au_server_socket::au_server_socket(hostname a, au_stream_port port_number):
    addr(a), port(port_number) {
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_AU)) == -1) {
        perror("AU server: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    memset(buffer, 0, AU_BUF_SIZE);
}

void au_socket::send(const void* buf, size_t size) {
    check_socket_set();

    // Address resolution stuff
    struct sockaddr_in sin;
    memset (&sin, 0, sizeof (struct sockaddr_in));
    get_remote_sockaddr(&sin);

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
    int sent = 0;
    size_t hdr_len = sizeof(struct tcphdr);
    size_t data_len = hdr_len + strlen(buffer + hdr_len);
    sent = ::sendto(sockfd, buffer, data_len, 0,
                    (struct sockaddr *)(&sin), sizeof(sin));
    if(sent == -1) {
        perror("AU Socket: error while sending");
        throw std::runtime_error("AU Socket: error while sending");
    }
    cerr << "sent " << sent << " bytes of data_len " << data_len << endl;
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
        cerr << "meow " << is_ours() << endl;
    } while(!is_ours());
    size_t iphdrlen = sizeof(struct ip);
    size_t tcphdrlen = sizeof(struct tcphdr);
    memcpy(buf, buffer + iphdrlen + tcphdrlen, std::min((size_t)(res - iphdrlen - tcphdrlen), size));

    cerr << "received " << res << " bytes of data:" << endl;
    /* }
     */
}

void au_client_socket::connect() {
    check_socket_set();

}

stream_socket* au_server_socket::accept_one_client() {
    return NULL;
}
