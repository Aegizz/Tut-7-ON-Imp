# Implementation of Olaf-Neighbourhood Protocol for Tutorial 7

Use the makefile for compiling.

Run these commands to be able to use the websocketpp and nlohmann JSON C++ libraries.

```bash
sudo apt-get install libboost-all-dev && sudo apt-get install libssl-dev && sudo apt-get install zlib1g-dev

git submodule update --init

cd websocketpp

mkdir build

cd build

cmake ..

sudo make install

cd ..

cd ..

cd json

mkdir build

cd build

cmake ..

sudo make install
```

# Running Clients and Servers
- Currently there exists server, server2 and server3.
  
    - server runs on ws://localhost:9002, server1 runs on ws://localhost:9003 and server3 runs on ws://localhost:9004
  
    - To compile servers, run ```make servers```
  
- Currently there exists testClient, testClient2 and testClient3 as well as userClient (which takes input from stdin)
  
    - testClient connects to server, testClient2 connects to server2 and testClient3 connects to server3
  
    - To compile test clients, run ```make testClients```
  
    - To compile userClient, run ```make userClient```
  

# Current Implemented Features
- Client <-> Server communication with multiple clients able to connect to a server
- Server <-> Server communication with multiple clients and servers able to connect to each server
- Client sends hello message when connecting with public key (implemented but not integrated RSA yet) (signed data not integrated yet)
- Client sends client list request
- Client generates UTC timestamp in ISO 8601 format for TTD in broadcasted packets
    - TTD not implemented yet
- Server can process hello message from client (signed data not integrated yet)
- Server can process server_hello message from server (signed data not integrated yet)
- Server can send client list to client
- Server can request client update from server
- Server can send client update to server

# Vulnerable Code

### Vulnerability #1
 In server.cpp, there is an insecure string copy which can cause a stack overflow, luckily it is protected by the compiler.... except that the developer disable stack protection and memory protection for debugging!!! Oh no!
    Insecure Copy /server.cpp Line 115
    server-debug Makefile Line 40

### Vulnerability #2
 In the gitignore, there is not ignorance of .pem files, the files used for key generation. This will likely lead to a user or users leaking keys at some point.
 This is a common way that users leak private information on the internet and has potential to cause issues later down the line as commit history cannot be removed.
### Vulnerability #3
 Oops! We forgot to discard messages from users where the signature does not match, the message will still be forwarded provided it can be decoded and will identify the user based on their client-id and server-id.
