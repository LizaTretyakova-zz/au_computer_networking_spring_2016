#include "logging.h"

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
