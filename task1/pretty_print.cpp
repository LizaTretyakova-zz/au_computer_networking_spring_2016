#include "au_socket.h"
#include "pretty_print.h"

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

void print_ip_header(unsigned char* buffer) {
    struct iphdr *iph = (struct iphdr *)buffer;
    struct sockaddr_in source;
    struct sockaddr_in dest;

    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;

    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;

    fprintf(stderr,"\n");
    fprintf(stderr,"IP Header\n");
    fprintf(stderr,"   |-IP Version        : %d\n", (unsigned int)iph->version);
    fprintf(stderr,"   |-IP Header Length  : %d DWORDS or %d Bytes\n", (unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
    fprintf(stderr,"   |-Type Of Service   : %d\n", (unsigned int)iph->tos);
    fprintf(stderr,"   |-IP Total Length   : %d  Bytes(Size of Packet)\n", ntohs(iph->tot_len));
    fprintf(stderr,"   |-Identification    : %d\n", ntohs(iph->id));
    //fprintf(stderr,"   |-Reserved ZERO Field   : %d\n", (unsigned int)iphdr->ip_reserved_zero);
    //fprintf(stderr,"   |-Dont Fragment Field   : %d\n", (unsigned int)iphdr->ip_dont_fragment);
    //fprintf(stderr,"   |-More Fragment Field   : %d\n", (unsigned int)iphdr->ip_more_fragment);
    fprintf(stderr,"   |-TTL      : %d\n", (unsigned int)iph->ttl);
    fprintf(stderr,"   |-Protocol : %d\n", (unsigned int)iph->protocol);
    fprintf(stderr,"   |-Checksum : %d\n", ntohs(iph->check));
    fprintf(stderr,"   |-Source IP        : %s\n", inet_ntoa(source.sin_addr));
    fprintf(stderr,"   |-Destination IP   : %s\n", inet_ntoa(dest.sin_addr));
}

void print_data(unsigned char* data , int size) {
    for(int i = 0 ; i < size ; i++) {
        if(i != 0 && i % 16 == 0) { //if one line of hex printing is complete...
            fprintf(stderr,"         ");
            for(int j = i - 16 ; j < i ; ++j)
            {
                if(data[j] >= 32 && data[j] <= 128) {
                    fprintf(stderr,"%c", (unsigned char)data[j]); //if its a number or alphabet
                } else {
                    fprintf(stderr, "."); //otherwise print a dot
                }
            }
            fprintf(stderr,"\n");
        }

        if(i % 16 == 0) {
            fprintf(stderr,"   ");
            fprintf(stderr," %02X", (unsigned int)data[i]);
        }

        if(i == size - 1) { //print the last spaces
            for(int j = 0; j < 15 - i % 16; ++j) {
                fprintf(stderr, "   "); //extra spaces
            }
            fprintf(stderr, "         ");
            for(int j = i - i % 16; j <= i; ++j) {
                if(data[j] >= 32 && data[j] <= 128) {
                    fprintf(stderr, "%c", (unsigned char)data[j]);
                } else {
                    fprintf(stderr, ".");
                }
            }
            fprintf(stderr, "\n");
        }
    }
}

void print_au_packet(unsigned char* buffer, int size) {
    unsigned short iphdrlen;

    struct iphdr *iph = (struct iphdr *)buffer;
    iphdrlen = iph->ihl * 4;

    struct tcphdr *tcph = (struct tcphdr*)(buffer + iphdrlen);

    fprintf(stderr, "\n\n***********************TCP Packet*************************\n");

    print_ip_header(buffer);

    fprintf(stderr, "\n");
    fprintf(stderr, "TCP Header\n");
    fprintf(stderr, "   |-Source Port      : %u\n", ntohs(tcph->source));
    fprintf(stderr, "   |-Destination Port : %u\n", ntohs(tcph->dest));
    fprintf(stderr, "   |-Sequence Number    : %u\n", ntohl(tcph->seq));
    fprintf(stderr, "   |-Acknowledge Number : %u\n", ntohl(tcph->ack_seq));
    fprintf(stderr, "   |-Header Length      : %d DWORDS or %d BYTES\n", (unsigned int)tcph->doff, (unsigned int)tcph->doff*4);
    //fprintf(stderr, "   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
    //fprintf(stderr, "   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
    fprintf(stderr, "   |-Urgent Flag          : %d\n", (unsigned int)tcph->urg);
    fprintf(stderr, "   |-Acknowledgement Flag : %d\n", (unsigned int)tcph->ack);
    fprintf(stderr, "   |-Push Flag            : %d\n", (unsigned int)tcph->psh);
    fprintf(stderr, "   |-Reset Flag           : %d\n", (unsigned int)tcph->rst);
    fprintf(stderr, "   |-Synchronise Flag     : %d\n", (unsigned int)tcph->syn);
    fprintf(stderr, "   |-Finish Flag          : %d\n", (unsigned int)tcph->fin);
    fprintf(stderr, "   |-Window         : %d\n", ntohs(tcph->window));
    fprintf(stderr, "   |-Checksum       : %d\n", ntohs(tcph->check));
    fprintf(stderr, "   |-Urgent Pointer : %d\n", tcph->urg_ptr);
    fprintf(stderr, "\n");
    fprintf(stderr, "                        DATA Dump                         ");
    fprintf(stderr, "\n");

    fprintf(stderr, "IP Header\n");
    print_data(buffer, iphdrlen);

    fprintf(stderr, "TCP Header\n");
    print_data(buffer + iphdrlen, tcph->doff * 4);

    fprintf(stderr, "Data Payload\n");
    print_data(buffer + iphdrlen + tcph->doff * 4, (size - tcph->doff * 4 - iph->ihl * 4));

    fprintf(stderr, "\n###########################################################");
}

void process_packet_pp(unsigned char* buffer, int size) {
    //Get the IP Header part of this packet
    struct iphdr *iph = (struct iphdr*)buffer;
    switch (iph->protocol) { //Check the Protocol and do accordingly...
        case IPPROTO_AU:
            cerr << "caight my packet" << endl;
            print_au_packet(buffer, size);
            break;
        default: //Some Other Protocol like ARP etc.
            cerr << "caught _smth_; that has a number of " << iph->protocol << endl;
            break;
    }
}
