#ifndef LOGGING
#define LOGGING

#include <string>

void log(const char* msg);
void log(std::string msg);
void log(std::string tag, struct my_tcphdr* tcph);

#endif
