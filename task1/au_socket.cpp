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
    set_fin(&tcph);

    send_packet(&tcph, (char*)NULL, 0, (struct sockaddr*)&remote_addr);

    ::close(sockfd);

    log("[CLOSE] closed");
}

void au_socket::send(const void* buf, size_t size) {
    log("[SEND]");
    check_socket_set();

    struct iphdr iph;
    struct my_tcphdr tcph;
    set_hdr(&tcph, local_port, remote_port);
    size_t nwords = std::min(size, AU_BUF_CAPACITY);
    log("[SEND] nwords=" + std::to_string(nwords));
    tcph.t.th_sum = checksum((unsigned char*)buf, nwords);
    log("[SEND] th_sum=" + std::to_string(tcph.t.th_sum));

    // send
    bool need_send = true;
    while(need_send) {
        log("[SEND] while");
        need_send = false;

        time_t send_time = time(NULL);
        send_packet(&tcph, (char*)buf, size, (struct sockaddr*)&remote_addr);

        // catch the ack
        while(true) {
            log("[SEND] catching ack");

            recv_packet(&iph, &tcph, NULL, 0);

            if(is_ours(&iph, &tcph)) {
                if(is_ack(&tcph)) {
                    log("[SEND] got an ack");
                    update_rtt(difftime(time(NULL), send_time));
                    break;
                } else if(is_fin(&tcph)) {
                    log("[SEND] connection closed");
                    ::close(sockfd);
                    sockfd = -1;
                    break;
                }
            }

            log("[SEND] missed");

            double diff = difftime(time(NULL), send_time);
            if(diff > RTO) {
                log("[SEND] reached ack timeout " + std::to_string(diff) + ". Retry.");
                need_send = true;
                break;
            }
            log("[SEND] diff="  + std::to_string(diff));
        }
    }
    log("[SEND] successfully sent: " + string((char*)buf));
}

void au_socket::recv(void *buf, size_t size) {
    log("[RECV]");
    check_socket_set();

    struct iphdr iph;
    struct my_tcphdr tcph;

    // receive message
    do {
        recv_packet(&iph, &tcph, (char*)buf, size);
    } while(!is_ours(&iph, &tcph));

    // process message
    size_t nwords = stream.get_last_read_words();
    log("[RECV] nwords=" + std::to_string(nwords));
    unsigned short csum = checksum((unsigned char*)buf, nwords);
    log("[RECV] csum=" + std::to_string(csum));
    if(tcph.t.th_sum == csum) {
        // everything is fine,
        // so give the message to user and send ack
        struct my_tcphdr response;
        set_ack(&response, ntohl(tcph.t.th_seq) + 1);

        send_packet(&response, (char*)NULL, 0, (struct sockaddr*)&remote_addr);
        log("[RECV] successfully received: " + string((char*)buf));
    } else {
        log("[RECV] checksum mismatch: th_sum=" + std::to_string(tcph.t.th_sum)
            + " csum=" + std::to_string(csum));
    }
}

void au_client_socket::connect() {
    log("[CONNECTING] connecting");

    // send syn
    unsigned int c_seq = rand();
    struct iphdr iph;
    struct my_tcphdr tcph;
    set_syn(&tcph, c_seq);

    time_t send_time = time(NULL);

    send_packet(&tcph, (char*)NULL, 0, (struct sockaddr*)&remote_addr);

    log("[CONNECTING] sent syn packet");

    // catch syn_ack
    do {
        log("[CONNECTING] catching syn_ack");
        recv_packet(&iph, &tcph, NULL, 0);
    } while(!is_ours(&iph, &tcph) || !from(&remote_addr, &iph) || !is_syn_ack(&tcph));

    log("[CONNECTING] caught syn_ack");

    time_t resp_time = time(NULL);
    set_rtt(difftime(resp_time, send_time));

    // check the correctness
    if(tcph.t.th_ack != c_seq + 1) {
        log("Error connecting to server: wrong seq");
        throw std::runtime_error("Wrong seq on client from server");
    }
    au_stream_port new_remote_port = tcph.small_things;

    // happily acknowledge
    set_ack(&tcph, tcph.t.th_seq + 1);
    tcph.t.th_ack = 0;

    send_packet(&tcph, NULL, 0, (struct sockaddr*)&remote_addr);

    state = CONNECTED;
    remote_port = new_remote_port;
    log("Client: connected");
}

stream_socket* au_server_socket::accept_one_client() {
    log("[ACCEPTING] accepting");

    struct iphdr iph;
    struct my_tcphdr tcph;

    // catch a syn packet
    do {
        log("[ACCEPTING] catching syn packet");
        recv_packet(&iph, &tcph, NULL, 0);
    } while(!to_this_server(&iph, &tcph) || !is_syn(&tcph));

    log("[ACCEPTING] caught syn packet");

    // remember sender's addr
    struct sockaddr_in client;
    memset(&client, 0, sizeof(struct sockaddr_in));
    client.sin_addr.s_addr = iph.saddr;
    client.sin_family = AF_INET;
    client.sin_port = tcph.t.th_sport;

    // respond with a syn_ack packet
    unsigned int s_seq = rand();
    au_stream_port new_port = rand();
    struct my_tcphdr response;
    set_syn_ack(&response, &tcph, s_seq, new_port);

    time_t send_time = time(NULL);
    send_packet(&response, NULL, 0, (struct sockaddr*)&client);

    // catch an ack packet
    do {
        recv_packet(&iph, &tcph, NULL, 0);
    } while(!to_this_server(&iph, &tcph) || !from(&client, &iph) || !is_ack(&tcph));

    time_t resp_time = time(NULL);

    if(tcph.t.th_seq == s_seq + 1) {
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
