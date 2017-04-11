#include "protocol.h"
#include "serialization.h"
#include "tcp_socket.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::vector;

// void make_request(tcp_client_socket* socket, uint8_t cmd, string argument, char* data = NULL, uint32_t data_size = 0) {
void make_request(tcp_client_socket* socket, uint8_t cmd, string argument, string data = "") {
    cerr << "[client]: request " << argument << " " << data << "\n";

    request req(cmd, argument, data);
    response res;
	
	send_request(socket, &req);
	recv_response(socket, &res);
	
    cerr << "[client]: command status: " << res.status_ok << "\n";
    if(cmd == static_cast<uint8_t>(GET)) {
        cerr << "[client]: got file contents\n" << res.data << "\n";
    }
}


int main(int argc, char* argv[]) {
    if (argc != 2 + 1 && argc != 0 + 1) {
        cerr << "RemoteFS client usage: <utility> <hostname> <port>\n";
        cerr << "Commands:<number> <argument>\n";
        cerr << "Command numbers:\n";
        cerr << "    0 -- cd\n";
        cerr << "    1 -- ls\n";
        cerr << "    2 -- get\n";
        cerr << "    3 -- put\n";
        cerr << "    4 -- del\n";
        exit(1);
    }

	hostname addr = DEFAULT_ADDR;
	tcp_port port = DEFAULT_PORT;
	if(argc == 2) {
		addr = argv[1];
		port = argv[2];
	}

	tcp_client_socket sock(addr, port);
	try {
		sock.connect();
        cout << "[client] connected\n";
    } catch(std::runtime_error e) {
        cerr << "Error connecting to server:\n" << e.what() << "\n";
		exit(1);
    } catch(...) {
        cerr << "Error connecting to server\n";
        exit(1);
    }

    int cmd;
    cin >> cmd;

    string argument;
    string cont_arg;
    cin >> argument;

    cerr << "[client]: got command " << cmd << " and argument " << argument << "\n";
    if(cmd == static_cast<uint8_t>(PUT)) {
        cin >> cont_arg;
        cerr << "... and for PUT command " << cont_arg << "\n";
    }

    try {
        if(cmd == static_cast<uint8_t>(PUT)) {
            make_request(&sock, (uint8_t)cmd, argument, cont_arg);
        } else {
            make_request(&sock, (uint8_t)cmd, argument);
        }
    } catch(std::runtime_error e) {
        cerr << "Error interacting with server: \n" << e.what() << "\n";
        exit(1);
    } catch(...) {
        cerr << "Error interacting with server\n";
        exit(1);
    }
}
