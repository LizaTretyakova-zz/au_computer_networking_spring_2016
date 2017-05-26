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
using std::endl;
using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;
using namespace boost::filesystem;


static void* process_client(void* arg) {
    tcp_socket* client = (tcp_socket*)arg;
    request req;
    response res;
    try {
        recv_request(client, &req);
    } catch(...) {
        delete client;
        cerr << "[server] error receiving client's request\n";
        return NULL;
    }
    cmd_code cmd = (cmd_code)req.cmd;

    try {
        switch(cmd) {
        case CD: {
            cerr << "[server]: cd " << req.target_path << "\n";
            current_path(path(req.target_path));
            res.status_ok = true;
            break;
        }
        case LS: {
            path p(req.target_path);
            cerr << "[server]: ls " << req.target_path << "\n";
            ostringstream output;
            if(is_directory(p)) {
                output << p << " is a directory containing:" << endl;

                for(auto& entry : boost::make_iterator_range(directory_iterator(p), {})) {
                    output << entry << endl;
                }
            }
            res.status_ok = true;
            res.data = output.str();
            break;
        }
        case GET: {
            // I allow only text files here
            cerr << "[server]: get " << req.target_path << "\n";

            ifstream in(req.target_path);
            stringstream sstr;

            sstr << in.rdbuf();
            res.data = sstr.str();
            res.status_ok = true;

            break;
        }
        case PUT: {
            cerr << "[server]: put " << req.target_path << " " << req.data << "\n";

            ofstream out(req.target_path);
            out << req.data;

            res.status_ok = true;
            break;
        }
        case DEL: {
            cerr << "[server]: del " << req.target_path << "\n";

            remove(path(req.target_path));
            res.status_ok = true;
            break;
        }
        default: {
            cerr << "[server]: client's request didn't match any command\n";
            break;
        }
        }
    } catch(std::runtime_error& e) {
        // we shouldn't drop server if the error is actually client's fault
        cerr << "[server]: thrown runtime error: " << e.what() << "\n";
        res.status_ok = false;
    } catch(...) {
        // okay, something really bad happened -- not trying to save client
        return NULL;
    }
    send_response(client, &res);

    delete client;
    cout << "[server] finished with client" << endl;
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
            cout << "[server] client connected" << endl;
            int res = pthread_create(&th, NULL, process_client, client);
            if (res < 0) {
                perror("[server]");
                exit(1);
            }
            pthread_detach(th);
        } catch(std::runtime_error& e) {
            cerr << "Error accepting client: \n" << e.what() << "\n";
            exit(1);
        } catch(...) {
            cerr << "Error accepting client\n";
            exit(1);
        }
	}
}
