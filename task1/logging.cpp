#include "logging.h"
#include "protocol.h"

#include <iostream>
#include <mutex>
#include <string>

std::mutex mtx;

void log(const char *msg) {
    mtx.lock();
    std::cerr << std::string(msg) << std::endl;
    mtx.unlock();
}

void log(std::string msg) {
    mtx.lock();
    std::cerr << msg << std::endl;
    mtx.unlock();
}

void log(std::string tag, struct my_tcphdr* tcph) {
    mtx.lock();
    std::cerr << tag << " th_sport:" + std::to_string(tcph->t.th_sport)
              << " th_dport:" << std::to_string(tcph->t.th_dport) << std::endl;
    mtx.unlock();
}
