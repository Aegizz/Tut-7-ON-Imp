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
    - Hasn't been implemented in private chats as of yet.
  - We also have some code to generate a time to die, however it has not been implemented yet.

# Clients and Servers

- Currently for our testing there exists server, server2 and server3.
  
    - server runs on ws://localhost:9002, server1 runs on ws://localhost:9003 and server3 runs on ws://localhost:9004
  
    - To compile servers, run ```make servers```

- See 'How to set up new Servers below' to see what files need to be changed to modify the neighbourhood.
  
- Currently there exists userClient (which takes input from stdin)
  -  We have been using testClient's for automated testing.
    - To compile userClient, run ```make userClient```

- Servers maintain their keys in the server-files directory as private_key_serverX.pem and public_key_serverX.pem
- Clients maintain their keys in the client directory as private_keyX.pem and public_keyX.pem

# How to set up new Servers
To set up new servers in the neighbourhood there are a few important files to change.

- server-files/neighbourhood_mapping.json contains valid public keys belonging to the servers stored against their server ID (a value you decide) stored in JSON form.
- In each serverX.cpp file where X is the ID of the server, there exists a const int ServerID, listenPort and a const std::string myAddress.
  - It is important that these are updated to match the current neighbourhood setup.
- In server-files/server_list.h, a private member of the ServerList class serverAddresses stores the IP+Port combination of the websocket server against the server ID.
  - It is important that this matches the neighbourhood mapping so connections can be established and maintained properly.
- In the server-files directory, each server maintains a server_mappingX.json file where X is the ID of the server. These store the IDs of clients of that server against their public keys.
  - If you want to create new ID mappings, delete the existing clients in the mapping. New clients will be added to the JSON file when connecting for the first time so these files don't need to be manually changed to allow new clients to connect but if you want to use IDs that are already assigned then modification will be required.
  - You will need to create a new call in the Makefile for serverY identical to server but replacing the dependency of server.cpp with serverY.cpp.
 
# How to set up new Clients
- In userClient there exists a const int clientNumber which is for their local client number X (nothing to do with their ClientID), this is important for key management.  
- Deleting client/private_keyX.pem and client/public_keyX.pem where X is the local client number (e.g. testClientX) will regenerate a client's keys when running userClient.
- Run ```cp userClient userClientY.cpp``` to create another user client.
- You will need to create a new call in the Makefile for userClientY identical to userClient but replacing the dependency of userClient.cpp with userClientY.cpp.

 # How to use the userClient and Server
 Run ```./server``` or ```./serverX``` where X is the number of the server process.
 
 Run ```./userClient``` or ```./userClientX``` where X is the number of the server process to start the userClient.
 - connect ws_uri
 - send message_type connection_id
   - Message types are private and public
     - If private, it will prompt you for Server IDs and Client IDs of recipients
 - close connection_id close_code:default=1000 close_reason
 - show connection_id
 - help: Display this help text
 - quit: Exit the program
  

# Current Implemented Features
- Client <-> Server communication with multiple clients able to connect to a server
- Server <-> Server communication with multiple clients and servers able to connect to each server
- Note: Signing is not fully implemented as counter is not implemented yet
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
