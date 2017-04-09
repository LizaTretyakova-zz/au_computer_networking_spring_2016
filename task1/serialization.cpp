#include "serialization.h"
#include "tcp_socket.h"

#include <cstdlib>
#include <cstring>


void send_request(tcp_socket* socket, request* req) {
	size_t len = strlen(req->target_path);
	uint32_t len_n = htonl(len);
	uint32_t data_size_n = htonl(req->data_size);

	socket->send(&req->cmd, sizeof(cmd_code)); // no need for htons -- value too short
	socket->send(&len_n, sizeof(uint32_t));
	socket->send(req->target_path, len);
	socket->send(&data_size_n, sizeof(uint32_t));
	socket->send(req->data, req->data_size);
}

void send_response(tcp_socket* socket, response* res) {
	uint32_t size = htonl(res->data_size);

	socket->send(&res->status);
	socket->send(&size, sizeof(uint32_t));
	socket->recv(res->data, res->data_size);
}

void recv_request(tcp_socket* socket, request* req) {
	uint32_t len;

	socket->recv(&req->cmd, sizeof(cmd_code));
	socket->recv(&len, sizeof(uint32_t));
	len = ntohl(len);
	req->target_path = (char*)malloc(len * sizeof(char));
	socket->recv(req->target_path, len);
	socket->recv(&req->data_size, sizeof(uint32_t));
	req->data_size = ntohl(req->data_size);
	req->data = (char*)malloc(req->data_size * sizeof(char));
	socket->recv(req->data, req->data_size);
}

void recv_response(tcp_socket* socket, response* res) {
	socket->recv(&res->status);
	socket->recv(&res->data_size, sizeof(uint32_t));
	res->data_size = ntohl(res->data_size);
	res->data = (char*)malloc(res->data_size * sizeof(char));
	socket->recv(res->data, res->data_size);	
}
