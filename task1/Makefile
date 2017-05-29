.PHONY: all clean

# Based on https://github.com/yeputons/project-templates

TARGETS=test client server
SRCS_common=$(SRCDIR)/tcp_socket.cpp $(SRCDIR)/au_socket.cpp $(SRCDIR)/pretty_print.cpp
SRCS_serializ = $(SRCDIR)/serialization.cpp
SRCS_test=$(SRCS_common) $(SRCDIR)/test.cpp
SRCS_client=$(SRCS_common) $(SRCS_serializ) $(SRCDIR)/client.cpp
SRCS_server=$(SRCS_common) $(SRCS_serializ) $(SRCDIR)/server.cpp
BINDIR=bin
SRCDIR=.
INCDIR=inc

all: $(TARGETS)
test: $(SRCS_test:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)
client: $(SRCS_client:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)
server: $(SRCS_server:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)

CXX=g++
CXXFLAGS=-pthread -pedantic -std=c++11 -ggdb -Wall -Wshadow -Wextra -Werror -std=c++11 -I$(INCDIR)
LDFLAGS=-pthread -ggdb 
LDLIBS= -lboost_system -lboost_filesystem
ifeq ($(OS),Windows_NT)
  CXXFLAGS+=-DWINVER=WindowsXP
  LDLIBS+=-lws2_32
endif

$(TARGETS):
	$(CXX) -o $@ $(LDFLAGS) $^ $(LDLIBS)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm -rf $(TARGETS) $(BINDIR)

include $(wildcard $(BINDIR)/*.d)
