# Define the compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -std=c++11

# Define the libraries
LIBS = -lssl -lcrypto -pthread

CLIENT_FILES=client/client_list.cpp
# Targets
all: client server test-client

test: debug-all server client testClient test.sh
	chmod +x test.sh
	bash test.sh

client: client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

testClient: testClient.cpp
	$(CXX) $(CXXFLAGS) -o testClient $^ $(LIBS) $(CLIENT_FILES)

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
