#include "serialization.h"
#include "tcp_socket.h"

#include <cstdlib>
#include <cstring>


void send_request(tcp_socket* socket, request* req) {
    // size_t len = strlen(req->target_path);
    size_t len = req->target_path.size();
    uint32_t len_n = htonl(len);
    // uint32_t data_size_n = htonl(req->data_size);
    uint32_t data_size_n = htonl(req->data.size());

    socket->send(&req->cmd, sizeof(uint8_t)); // no need for htons -- value too short
	socket->send(&len_n, sizeof(uint32_t));
    socket->send(req->target_path.c_str(), len);
	socket->send(&data_size_n, sizeof(uint32_t));
//    socket->send(req->data.c_str(), req->data_size);
    socket->send(req->data.c_str(), req->data.size());
}

void send_response(tcp_socket* socket, response* res) {
//	uint32_t size = htonl(res->data_size);
    uint32_t size = htonl(res->data.size());

    socket->send(&res->status_ok, sizeof(bool));
    socket->send(&size, sizeof(uint32_t));
//    socket->send(res->data.c_str(), res->data_size);
    socket->send(res->data.c_str(), res->data.size());
}

void recv_request(tcp_socket* socket, request* req) {
	uint32_t len;

    socket->recv(&req->cmd, sizeof(uint8_t));
	socket->recv(&len, sizeof(uint32_t));
	len = ntohl(len);
//	req->target_path = (char*)malloc(len * sizeof(char));
    req->target_path.resize(len);
    socket->recv(&(req->target_path)[0], len);
//	socket->recv(&req->data_size, sizeof(uint32_t));
//    req->data_size = ntohl(req->data_size);
    socket->recv(&len, sizeof(uint32_t));
    len = ntohl(len);
//    req->data = (char*)malloc(req->data_size * sizeof(char));
    req->data.resize(len);
//    socket->recv(&(req->data)[0], req->data_size);
    socket->recv(&(req->data)[0], req->data.size());
}

void recv_response(tcp_socket* socket, response* res) {
    socket->recv(&res->status_ok, sizeof(bool));
//	socket->recv(&res->data_size, sizeof(uint32_t));
    uint32_t len;
    socket->recv(&len, sizeof(uint32_t));
//    res->data_size = ntohl(res->data_size);
    len = ntohl(len);
//	res->data = (char*)malloc(res->data_size * sizeof(char));
    res->data.resize(len);
//    socket->recv(res->data, res->data_size);
    socket->recv(&(res->data)[0], len);
}
