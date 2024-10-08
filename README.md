# Implementation of Olaf-Neighbourhood Protocol for Tutorial 7

To compile the code the following commands will need to be run to install the websocketpp and nlohmann JSON C++ libraries required to run our implementation.

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

We are using a makefile for compiling our code.

Our implementation is modified to heavily use server and client IDs to simplify the identification of servers and clients.
  - We are sending them in client lists, client updates, and private chats.
  - We also have some code to generate a time to die, however it has not been implemented yet.
  - 
# Running Clients and Servers

- Currently for our testing there exists server, server2 and server3.
  
    - server runs on ws://localhost:9002, server1 runs on ws://localhost:9003 and server3 runs on ws://localhost:9004
  
    - To compile servers, run ```make servers```
  
- Currently there exists testClient, testClient2 and testClient3 as well as userClient (which takes input from stdin)
  
    - testClient connects to server on ws://localhost:9002, testClient2 connects to server2 on ws://localhost:9003 and testClient3 connects to server3 on ws://localhost:9004.
  
    - To compile test clients, run ```make testClients```
  
    - To compile userClient, run ```make userClient```

# How to set up new Servers
To set up new servers in the neighbourhood there are a few important files to change.

- server-files/neighbourhood_mapping.json contains valid public keys belonging to the servers stored against their server ID (a value you decide) stored in JSON form.
- In server-files/server_list.h, a private member of the ServerList class serverAddresses stores the IP+Port combination of the websocket server against the server ID.
  - It is important that both of these matching so connections can be established and maintained properly.
- In each serverX.cpp file where X is the ID of the server, there exists a const int ServerID, listenPort and a const std::string myAddress.
  - It is important that these are updated to match the current neighbourhood setup.
 
# How to set up new Clients
- To set up new clients, delete private_keyX.pem and public_keyX.pem where X is the local client number (e.g. testClientX) to regenerate their keys.
  - Alternatively run cp testClientX.cpp testClientY.cpp to create a new client.
- Each client has a const int clientNumber which matches their local client number X.    
- Currently there is no front end to input what server the client will connect to, but in each testClient, there is a command with a manually entered URI for the server to connect to in the command "endpoint.connect". Changing this will change what server the client will connect to.
- In the server-files directory, each server maintains a server_mappingX.json file where X is the ID of the server. These store the IDs of clients of that server against their public keys.
  - If you want to create new ID mappings, these files need to be modified. New clients will be added to the JSON file when connecting for the first time so these files don't need to be manually changed to allow new clients to connect.   
  

# Current Implemented Features
- Client <-> Server communication with multiple clients able to connect to a server
- Server <-> Server communication with multiple clients and servers able to connect to each server
- Server sends server_hello message when connecting, signing with their private key
- Client sends hello message when connecting, signing the message and sending their public key
- Client sends client list request
- Client generates UTC timestamp in ISO 8601 format for TTD in broadcasted packets
    - TTD not implemented yet
- Server can process hello message from client and verify signature
- Server can process server_hello message from server and verify signature
- Server can send client list to client
- Server can request client update from server
- Server can send client update to server
- Client can send a public chat
- Client can send a private chat
- Server can forward a public chat to everyone in the neighbourhood
- Server can forward a private chat to all destination servers
- Client can handle a public chat and extract message
- Client can handle a private chat and decrypt the message if it is meant for them

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
### Vulnerability #4
No input validation is being run for test commands on the client or the server. Will allow buffer overflow vulnerabilities.
