#include "serialization.h"
#include "tcp_socket.h"

#include <cstdlib>
#include <cstring>


void send_request(tcp_socket* socket, request* req) {
    size_t len = req->target_path.size();
    uint32_t len_n = htonl(len);
    uint32_t data_size_n = htonl(req->data.size());

    socket->send(&req->cmd, sizeof(uint8_t)); // no need for htons -- value too short
    socket->send(&len_n, sizeof(uint32_t));
    socket->send(req->target_path.c_str(), len);
    socket->send(&data_size_n, sizeof(uint32_t));
    socket->send(req->data.c_str(), req->data.size());
}

void send_response(tcp_socket* socket, response* res) {
    uint32_t size = htonl(res->data.size());

    socket->send(&res->status_ok, sizeof(bool));
    socket->send(&size, sizeof(uint32_t));
    socket->send(res->data.c_str(), res->data.size());
}

void recv_request(tcp_socket* socket, request* req) {
	uint32_t len;

    socket->recv(&req->cmd, sizeof(uint8_t));
    socket->recv(&len, sizeof(uint32_t));
    len = ntohl(len);
    req->target_path.resize(len);
    socket->recv(&(req->target_path)[0], len);
    socket->recv(&len, sizeof(uint32_t));
    len = ntohl(len);
    req->data.resize(len);
    socket->recv(&(req->data)[0], req->data.size());
}

void recv_response(tcp_socket* socket, response* res) {
    socket->recv(&res->status_ok, sizeof(bool));
    uint32_t len;
    socket->recv(&len, sizeof(uint32_t));
    len = ntohl(len);
    res->data.resize(len);
    socket->recv(&(res->data)[0], len);
}

void socket_stream::put_hdr(struct my_tcphdr* hdr) {
    if(ptr + sizeof(struct my_tcphdr) >= BUF_SIZE) {
        throw std::runtime_error("[SOCKET_STREAM] put_hdr: not enough space");
    }

    memcpy(buffer + ptr, hdr, sizeof(struct my_tcphdr));
    struct my_tcphdr* tcph = (struct my_tcphdr*)(buffer + ptr);
    ptr += sizeof(struct my_tcphdr);

    tcph->t.th_sport = htons(tcph->t.th_sport);
    tcph->t.th_dport = htons(tcph->t.th_dport);
    tcph->t.th_seq = htonl(tcph->t.th_seq);
    tcph->t.th_ack = htonl(tcph->t.th_ack);
    // no conversion for th_x2 and th_off,
    // since they together form a single uint8_t;
    // same for th_flags
    tcph->t.th_win = htons(tcph->t.th_win);
    tcph->t.th_sum = htons(tcph->t.th_sum);
    tcph->t.th_urp = htons(tcph->t.th_urp);
    tcph->small_things = htons(tcph->small_things);
}

void socket_stream::put_data(const char* data, size_t size) {
    if(ptr + size >= BUF_SIZE) {
        throw std::runtime_error("[SOCKET_STREAM] put_data: not enough space");
    }
    memcpy(buffer, data, size);
}

void socket_stream::put_char(char c) {
    if(ptr + sizeof(char) >= BUF_SIZE) {
        throw std::runtime_error("[SOCKET_STREAM] put_char: not enough space");
    }
    buffer[ptr] = c;
    ptr += sizeof(char);
}

void socket_stream::get_data() {
    return buffer;
}

size_t get_size() {
    return ptr;
}

void socket_stream::reset() {
    ptr = 0;
    memset(buffer, 0, BUF_SIZE);
}
