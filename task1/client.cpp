#include "protocol.h"
#include "serialization.h"
#include "tcp_socket.h"

#include <iostream>
#include <stdexcept>
#include <string>

using std::cin;
using std::cout;
using std::cerr;
using std::string;

void make_request(tcp_client_socket* socket, cmd_code cmd, string argument, char* data = NULL, uint32_t data_size = 0) {
	request req(cmd, argument.c_str(), data, data_size);
	response res();
	
	send_request(socket, &req);
	recv_response(socket, &res);
	
	cout << "RemoteFS client command status: " << res.status << "\n";
	if(cmd == get) {
        cout << "Got file contents:\n" << string(res.data, res.data_size) << "\n";
	}
}


int main(int argc, char* argv[]) {
	if (argc != 2 && argc != 0) {
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

	cmd_code cmd;
	while(cin >> cmd) {
		string argument;
		string cont_arg;

		cin >> argument;
		if(cmd == put) {
			cin >> cont_arg;
		}

		try {
			if(cmd == put) {
                make_request(&sock, cmd, argument, cont_arg.c_str(), cont_arg.size());
			} else {
                make_request(&sock, cmd, argument);
			}
        } catch(std::runtime_error e) {
            cerr << "Error interacting with server: \n" << e.what() << "\n";
            exit(1);
        } catch(...) {
            cerr << "Error interacting with server\n";
            exit(1);
        }
	}
}
