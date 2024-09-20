# Define the compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -std=c++11

# Define the libraries
LIBS = -lssl -lcrypto -pthread

# Targets
all: client server

test: debug-all server client
	bash test.sh

client: client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

debug: debugClient.cpp
	$(CXX) $(CXXFLAGS) -o debugClient $^ $(LIBS)

server: server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) -lz

# Clean up build artifacts
clean:
	rm -f client server client-debug server-debug debugClient

debug-all: client-debug server-debug debug

client-debug: client.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)

server-debug: server.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS) -lz -fno-stack-protector
