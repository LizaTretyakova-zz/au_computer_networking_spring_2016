#include "au_socket.h"
#include "logging.h"
#include "pretty_print.h"
#include "stream_socket.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>            // strcpy, memset(), and memcpy()
#include <ctime>
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

    log("created socket with fd " + std::to_string(sockfd)
        + ", local_port " + std::to_string(cp)
        + ", remote_port " + std::to_string(sp)
        + " and state " + std::to_string(s));

    memset(buffer, 0, AU_BUF_SIZE);
    RTT = 0;
    SRTT = 0;
    RTO = 1;
}

au_socket::au_socket(int new_fd, hostname a, au_stream_port cp, au_stream_port sp, state_t s):
    au_socket(a, cp, sp, s) {
    sockfd = new_fd;
}

void au_socket::set_remote_addr() {
    get_sockaddr(addr, remote_port, &remote_addr);
}

void au_socket::close() {
    log("[CLOSE] port " + std::to_string(local_port));
    check_socket_set();

    struct my_tcphdr tcph;
    memset(&tcph, 0, sizeof(struct my_tcphdr));
    tcph.t.th_sport = local_port;
    tcph.t.th_dport = remote_port;
    tcph.t.th_sum = 0;
    tcph.t.th_flags = TH_FIN;

    if(::sendto(sockfd, &tcph, sizeof(struct my_tcphdr), 0,
                (struct sockaddr*)&remote_addr, sizeof(struct sockaddr)) < 0) {
        perror("AU Socket: error sending FIN");
        throw std::runtime_error("AU Socket: error sending FIN");
    }
    ::close(sockfd);

    log("[CLOSE] closed");
}

void au_socket::send(const void* buf, size_t size) {
    log("[SEND]");
    check_socket_set();

    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    // clear buffer
    memset(out_buffer, 0, AU_BUF_SIZE);

    // write tcphdr
    struct my_tcphdr* tcph = (struct my_tcphdr*)(out_buffer);
    tcph->t.th_sport = local_port;
    tcph->t.th_dport = remote_port;

    // write data
    int nwords = std::min(size, AU_BUF_CAPACITY);
    char* data = out_buffer + sizeof(struct my_tcphdr);
    memcpy(data, buf, nwords);
    out_buffer[AU_BUF_SIZE - 1] = 0;
    tcph->t.th_sum = checksum((unsigned short*)data, nwords);

    // send
    bool need_send = true;
    while(need_send) {
        log("[SEND] while");
        need_send = false;

        time_t send_time = time(NULL);
        if(::sendto(sockfd, out_buffer, sizeof(struct my_tcphdr) + nwords, 0,
                    (struct sockaddr *)(&remote_addr), sizeof(remote_addr)) < 0) {
            perror("AU Socket: error while sending");
            throw std::runtime_error("AU Socket: error while sending");
        }

        // catch the ack
        while(true) {
            if(recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size) < 0) {
                perror("AU Socket: error while receiving");
                throw std::runtime_error("AU Socket: error while receiving");
            }
            tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));

            if(is_ours()) {
                if(is_ack()) {
                    log("[SEND] got an ack");
                    update_rtt(difftime(time(NULL), send_time));
                    break;
                } else if(is_fin()) {
                    log("[SEND] connection closed");
                    ::close(sockfd);
                    sockfd = -1;
                    break;
                }
            }

            double diff = difftime(time(NULL), send_time);
            if(diff > RTO) {
                log("[SEND] reached ack timeout " + std::to_string(diff) + ". Retry.");
                need_send = true;
                break;
            }
        }
    }
    log("[SEND] successfully sent: " + string((char*)buf));
}

void au_socket::recv(void *buf, size_t size) {
    log("[RECV]");
    check_socket_set();

    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    // receive message
    int res;
    do {
        res = recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size);
        if(res < 0) {
            perror("AU Socket: error while receiving");
            throw std::runtime_error("AU Socket: error while receiving");
        }
    } while(!is_ours());

    // process message
    struct my_tcphdr *tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));
    int nwords = (int)std::min((size_t)(res - sizeof(struct ip) - sizeof(struct my_tcphdr)), size);
    unsigned short csum =
        checksum((unsigned short*)(buffer + sizeof(struct ip) + sizeof(struct my_tcphdr)), nwords);

    if(tcph->t.th_sum == csum) {
        // everything is fine, so

        // give the message to user...
        memcpy(buf, buffer + sizeof(struct ip) + sizeof(struct my_tcphdr), nwords);

        // ... and send ack
        struct my_tcphdr response;
        memset(&response, 0, sizeof(struct my_tcphdr));
        response.t.th_sport = local_port;
        response.t.th_dport = remote_port;
        response.t.th_seq = tcph->t.th_seq + 1;
        response.t.th_flags = TH_ACK;
        if(::sendto(sockfd, &response, sizeof(struct my_tcphdr), 0,
                    (struct sockaddr*)&remote_addr, sizeof(struct sockaddr)) < 0) {
            perror("AU Socket: failed to send ack");
            throw std::runtime_error("AU Socket: failed to send ack");
        }
        log("[RECV] successfully received: " + string((char*)buf));
    } else {
        log("[RECV] checksum mismatch!");
    }
}

void au_client_socket::connect() {
    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    log("Connecting");

    // send syn
    unsigned int c_seq = rand();
    struct my_tcphdr tcph;
    memset(&tcph, 0, sizeof(struct my_tcphdr));
    tcph.t.th_sport = local_port;
    tcph.t.th_dport = remote_port;
    tcph.t.th_seq = htonl(c_seq);
    tcph.t.th_off = 0; // sizeof(struct my_tcphdr) / 4;
    tcph.t.th_flags = TH_SYN;
    tcph.t.th_win = htonl(65535);
    tcph.t.th_sum = 0;

    time_t send_time = time(NULL);

    if(::sendto(sockfd, &tcph, sizeof(struct my_tcphdr), 0,
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

    time_t resp_time = time(NULL);
    set_rtt(difftime(resp_time, send_time));

    // check the correctness
    struct my_tcphdr* tcph_resp = (struct my_tcphdr*)(buffer + sizeof(struct ip));
    if(ntohl(tcph_resp->t.th_ack) != c_seq + 1) {
        log("Error connecting to server: wrong seq");
        throw std::runtime_error("Wrong seq on client from server");
    }
    au_stream_port new_remote_port = tcph_resp->small_things;

    // happily acknowledge
    tcph.t.th_seq = htonl(ntohl(tcph_resp->t.th_seq) + 1);
    tcph.t.th_ack = 0;
    tcph.t.th_flags = TH_ACK;

    if(::sendto(sockfd, &tcph, sizeof(struct my_tcphdr), 0,
                (struct sockaddr*)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("failed to send ack");
        throw std::runtime_error("AU Client Socket: failed to send ack");
    }

    state = CONNECTED;
    remote_port = new_remote_port;
    log("Client: connected");
}

stream_socket* au_server_socket::accept_one_client() {
    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    log("Accepting");

    // catch a syn packet
    do {
        if(recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size) < 0) {
            perror("AU Server Socket: error while receiving syn");
            throw std::runtime_error("AU Server Socket: error while receiving syn");
        }
    } while(!to_this_server() || !is_syn());

    // extract headers
    struct iphdr* iph = (struct iphdr*)buffer;
    struct my_tcphdr* tcph = (struct my_tcphdr*)(buffer + sizeof(struct ip));

    // remember sender's addr
    struct sockaddr_in client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    client.sin_addr.s_addr = iph->saddr;
    client.sin_family = AF_INET;
    client.sin_port = tcph->t.th_sport;

    // respond with a syn_ack packet
    unsigned int s_seq = rand();
    au_stream_port new_port = rand();
    struct my_tcphdr response;
    memcpy(&response, tcph, sizeof(struct my_tcphdr));
    response.t.th_sport = tcph->t.th_dport;
    response.t.th_dport = tcph->t.th_sport;
    response.t.th_seq = htonl(s_seq);
    response.t.th_ack = htonl(ntohl(tcph->t.th_seq) + 1);
    response.t.th_off = 0; // sizeof(struct my_tcphdr) / 4;
    response.t.th_flags = TH_SYN | TH_ACK;
    response.t.th_win = htonl(65535);
    response.t.th_sum = 0;
    response.small_things = new_port;

    time_t send_time = time(NULL);

    if(::sendto(sockfd, &response, sizeof(struct my_tcphdr), 0,
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

    time_t resp_time = time(NULL);

    if(ntohl(tcph->t.th_seq) == s_seq + 1) {
        // return tuned socket
        char peer_addr[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &(client.sin_addr), peer_addr, INET_ADDRSTRLEN) == NULL) {
            perror("accept_one_client -- could not process client addr");
            throw std::runtime_error("AU Server Socket: inet_ntop");
        }

        log("Server: accepted, peer addr " + string(peer_addr));
        au_socket* new_peer = new au_socket(peer_addr, new_port, client.sin_port, CONNECTED);
        new_peer->set_remote_addr();
        new_peer->set_rtt(difftime(resp_time, send_time));
        return new_peer;
    }

    log("Server: could not accept");
    return NULL;
}
