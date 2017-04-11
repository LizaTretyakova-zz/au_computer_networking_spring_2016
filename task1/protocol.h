#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

using std::string;

enum cmd_code: uint8_t {
    CD = 0,
    LS = 1,
    GET = 2,
    PUT = 3,
    DEL = 4
};

struct request {
    uint8_t cmd;
    string target_path;
    string data;
//  char* target_path;
//	char* data;
//	uint32_t data_size;

    request(uint8_t c = 0, string tp = "", string d = ""): //char* tp = NULL, char* d = NULL, uint32_t ds = 0):
        cmd(c), target_path(tp), data(d) {} // , data_size(ds) {}
};

struct response {
    bool status_ok;
    string data;
//  char* data;
//	uint32_t data_size;

    response(bool ok = false, string d = ""): // char* d = NULL, uint32_t ds = 0):
        status_ok(ok), data(d) {} // , data_size(ds) {}
};
