# Define the compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -std=c++11

# Define the libraries
LIBS = -lssl -lcrypto -pthread

CLIENT_FILES=client/client_list.cpp
SERVER_FILES=server-files/server_list.cpp
# Targets
all: client server server2 testClient testClient2 testClient3 test-client-client_list

test: debug-all server client testClient test.sh test-client-list
	echo "Running tests..."
	chmod +x test.sh
	bash test.sh
	echo "Running client tests..."
	./test-client-list

client: client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# For testing 
testClient: testClient.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(CLIENT_FILES)
testClient2: testClient2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(CLIENT_FILES)
testClient3: testClient3.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(CLIENT_FILES)
server: server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(SERVER_FILES) -lz
server2: server2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) $(SERVER_FILES) -lz

# Clean up build artifacts
clean:
	rm -f client-debug server-debug server server2 testClient testClient2 testClient3 tests/server.log tests/client.log debugClient

debug-all: client-debug server-debug

client-debug: client.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)

server-debug: server.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS) -lz -fno-stack-protector

test-client-list: tests/test_client_list.cpp client/client_list.h client/client_list.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)
