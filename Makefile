# Define the compiler
CXX = g++

# Define compiler flags
CXXFLAGS = -Wall -std=c++11

# Define the libraries
LIBS = -lssl -lcrypto -pthread

CLIENT_FILES=client/*.cpp
# Targets
all: userClient server test-client testClient

test: debug-all server client testClient test.sh test-client-list test-client-aes-encrypt test-client-sha256 test-client-key-gen test-base64
	echo "Running tests..."
	chmod +x test.sh
	bash test.sh
	echo "Running client tests..."
	./test-client-list
	./test-client-aes-encrypt
	./test-client-sha256
	./test-client-key-gen
	./test-base64

userClient: userClient.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

testClient: testClient.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CLIENT_FILES) $(LIBS)

server: server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) -lz

# Clean up build artifacts
clean:
	rm -f userClient server client-debug server-debug testClient tests/server.log tests/client.log debugClient test-client-sha256 test-client-aes-encrypt test-client-list test-base64 test-client-key-gen userClient userClient-debug

debug-all: userClient-debug testClient server-debug

userClient-debug: userClient.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS) -lz -fno-stack-protector

server-debug: server.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS) -lz -fno-stack-protector

test-client: test-client-list test-client-aes-encrypt test-client-sha256

test-client-list: tests/test_client_list.cpp client/client_list.h client/client_list.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)
test-client-aes-encrypt: tests/test_aes_encrypt.cpp client/aes_encrypt.cpp client/aes_encrypt.h
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)
test-client-sha256: tests/test_Sha256Hash.cpp client/Sha256Hash.cpp client/Sha256Hash.h
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)
test-client-key-gen: tests/test_client_key_gen.cpp client/client_key_gen.h client/client_key_gen.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)
test-base64: tests/test_base64.cpp client/base64.cpp
	$(CXX) $(CXXFLAGSR) -g -o $@ $^ $(LIBS)
test-client-signature: client/base64.cpp client/client_key_gen.cpp client/client_signature.cpp tests/test_client_signature.cpp client/Sha256Hash.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ $^ $(LIBS)