# Define the compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -std=c++11

# Define the libraries
LIBS = -lssl -lcrypto -pthread

# Targets
all: client server test-client

test: debug-all server client test-client test.sh
	chmod +x test.sh
	bash test.sh

client: client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

test-client: testClient.cpp
	$(CXX) $(CXXFLAGS) -o testClient $^ $(LIBS)

server: server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) -lz

# Clean up build artifacts
clean:
	rm -f client server client-debug server-debug testClient tests/server.log tests/client.log debugClient

debug-all: client-debug server-debug

client-debug: client.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)

server-debug: server.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS) -lz -fno-stack-protector
