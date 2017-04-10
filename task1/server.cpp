#include "protocol.h"
#include "serialization.h"
#include "tcp_socket.h"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

using std::cin;
using std::cout;
using std::cerr;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::vector;
using namespace boost::filesystem;


static void* process_client(void* arg) {
    tcp_socket* client = (tcp_socket*)arg;
    request req;
    response res;
    recv_request(client, &req);

    cmd_code cmd = (cmd_code)req.cmd;

    switch(cmd) {
    case CD: {
        cerr << "[server]: cd " << string(req.target_path) << "\n";
        current_path(path(req.target_path));
        res.status_ok = true;
        break;
    }
    case LS: {
        path p(req.target_path);
        cerr << "[server]: ls " << string(req.target_path) << "\n";
        if(is_directory(p)) {
            cerr << p << " is a directory containing:\n";

            for(auto& entry : boost::make_iterator_range(directory_iterator(p), {})) {
                cerr << entry << "\n";
            }
        }
        res.status_ok = true;
        break;
    }
    case GET: {
        cerr << "[server]: get " << string(req.target_path) << "\n";

        ifstream in(req.target_path);
        stringstream sstr;

        sstr << in.rdbuf();
        string str = sstr.str();
        vector<char> data_v(str.c_str(), str.c_str() + str.size() + 1);
        res.data = &data_v[0];
        res.data_size = data_v.size();
        res.status_ok = true;

        break;
    }
    case PUT: {
        cerr << "[server]: put " << string(req.target_path) << "\n";

        ofstream out(req.target_path);
        out << string(req.data, res.data_size);

        res.status_ok = true;
        break;
    }
    case DEL: {
        cerr << "[server]: del " << string(req.target_path) << "\n";

        remove(path(req.target_path));
        res.status_ok = true;
        break;
    }
    default: {
        cerr << "[server]: client's request didn't match any command\n";
        break;
    }
    }

    send_response(client, &res);
    free(req.data);
    return NULL;
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

	tcp_server_socket sock(addr, port);

	while(true) {
        try {
            pthread_t th;
            tcp_socket* client = reinterpret_cast<tcp_socket*>(sock.accept_one_client());
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
