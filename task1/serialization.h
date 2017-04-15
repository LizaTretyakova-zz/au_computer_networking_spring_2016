#pragma once

#include "protocol.h"
#include "tcp_socket.h"

void send_request(tcp_socket* socket, request* req);
void send_response(tcp_socket* socket, response* res);
void recv_request(tcp_socket* socket, request* req);
void recv_response(tcp_socket* socket, response* res);
