#include "protocol.h"
#include "serialization.h"
#include "tcp_socket.h"

#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>


using std::cin;
using std::cout;
using std::cerr;
using std::string;
using namespace boost::filesystem;


static void* process_client(void* arg) {
    tcp_client_socket* client = (tcp_client_socket*)arg;
    request req;
    response res;

    recv_request(client, &req);
    switch(req.cmd) {
    case cd:
        current_path(path(req.target_path));
        res.status = ok;
        break;
    case ls:
        path p(req.target_path);
        if(is_directory(p)) {
            cout << p << " is a directory containing:\n";

            for(auto& entry : boost::make_iterator_range(directory_iterator(p), {})) {
                cout << entry << "\n";
            }
        }
        res.status = ok;
        break;
    case get:
        ifstream in(req.target_path);
        stringstream sstr;

        sstr << in.rdbuf();
        res.data = sstr.str().c_str();
        res.data_size = strlen(contents);
        res.status = ok;

        break;
    case put:
        ofsteam out(req.target_path);
        out << string(req.data, res.data_size);

        res.status = ok;
        break;
    case del:
        remove(path(req.target_path));
        res.status = ok;
        break;
    }

    send_response(client, res);
    return;
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

	tcp_server_socket sock(addr, port);

	while(true) {
        try {
            pthread_t th;
            tcp_socket* client = sock.accept_one_client();
            cout << "[server] client connected\n";
            pthread_create(&th, NULL, process_client, client);
        } catch(std::runtime_error e) {
            cerr << "Error accepting client: \n" << e.what() << "\n";
            exit(1);
        } catch(...) {
            cerr << "Error accepting client\n";
            exit(1);
        }
	}
}
