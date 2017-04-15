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
