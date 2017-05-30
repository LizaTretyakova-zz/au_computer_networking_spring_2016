#ifndef PRETTY_PRINT
#define PRETTY_PRINT

#include "au_socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

using std::cerr;
using std::endl;
using std::string;

void print_ip_header(unsigned char* buffer);
void print_data(unsigned char* data , int size);
void print_au_packet(unsigned char* buffer, int size);
void process_packet_pp(unsigned char* buffer, int size);

#endif
