# Implementation of Olaf-Neighbourhood Protocol for Tutorial 7
[![CodeQL](https://github.com/Aegizz/Tut-7-ON-Imp/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/Aegizz/Tut-7-ON-Imp/actions/workflows/github-code-scanning/codeql)
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
  - We also have time to die's sent in our private chat messages.

# Compiling Servers

- In our current build, there exists three server files but they don't have any generated keys. server, server2 and server3.
  
    - Currently server runs on ws://localhost:9002, server1 runs on ws://localhost:9003 and server3 runs on ws://localhost:9004
  
    - To compile serverX, run ```make serverX``` or to compile all three servers, run ```make servers```
 
# Compiling Clients

- Currently there exists userClient, userClient2, userClient3 (which all take input from stdin)
    - To compile userClientX, run ```make userClientX``` or ```make userClients``` to compile all three clients.
      - We previously used the testClient files for automated testing.
     
 # How to use the userClient and Server
 Run ```./server``` or ```./serverX``` where X is the number of the server.
 
 Run ```./userClient``` or ```./userClientX``` where X is the number of the client. This will be suffixed on the key files created for the userClient.

 Note: A client can only make a connection to one server at a time.

 Command List:
 - connect
   - Prompts you for a uri, so enter a uri in the format "ws://IP:Port"
 - send message_type
   - Message types are private and public
     - If private, it will prompt you for Server IDs and Client IDs of recipients
 - close close_code:default=1000 close_reason
   - close_code and close_reason are optional, so just typing "close" is fine.  
 - show
   - Displays metadata about the connection
 - help: Displays command list
 - quit: Exit the program

# How to set up servers
Running the servers for the first time will generate the public and private keys, but they will not connect to each other as the neighbourhood is not set up.

To set up new servers in the neighbourhood there are a few important files to change.

- server-files/neighbourhood_mapping.json is currently empty as servers are not setup.
  - It should contain valid public keys belonging to the servers stored against their server ID (a value you decide) stored in JSON form.
- In server-files/ there will exist private and public keys belonging to the servers after keys are generated.
    - After generating keys which occurs when running a server for the first time, navigate to server-files directory and run ```./formatKeys.sh public_server_keyX.pem``` where X is the ID of the server. It will output the formatted keys as modified_public_key_serverX.txt.
    - Copy and paste the contents of the .txt files with "" around them into the neighbourhood_mapping.json file.
- In each serverX.cpp file where X is the ID of the server, there exists a const int ServerID, listenPort and a const std::string myAddress.
  - ServerID should be set to the ID of the server agreed upon by the neighbourhood.
  - listenPort should be set to the port you want the server to be listening on.
  - myAddress should be set to the address of the server IP+Port e.g. "127.0.0.1:9002".
  - It is important that these are updated to match the current neighbourhood setup.
 
- In server-files/server_list.h, a private member of the ServerList class serverAddresses stores the IP+Port combination of the websocket server against the server ID.
    - The serverAddresses variable will be declared but initialized with nothing. An example of how it looks is provided above the declaration. 
  - It is important that after server keys are generated this is updated to correctly match the neighbourhood mapping so connections are established to other servers and maintained properly.
- In the server-files directory, each server will maintain a server_mappingX.json file where X is the ID of the server. These store the IDs of clients of that server against their public keys.
    - These will not be present in the submission, and will be automatically created and populated when a user connects to the server for the first time. 
  - If you want to create new ID mappings, delete any existing clients in the mapping. New clients will be added to the JSON file when connecting for the first time so these files don't need to be manually changed to allow new clients to connect but if you want to use IDs that are already assigned then modification will be required.
- Run ```cp server serverX.cpp``` to create another user client.
- You will need to create a new call in the Makefile for serverX identical to server but replacing the dependency of server.cpp with serverX.cpp.
    - There are many dependant files so it is important to modify the Makefile for new servers.
 
# How to set up clients
- In userClient, there exists a const int clientNumber.
  - clientNumber is not related to ClientID, it is important for managing local client keys.    
- Deleting client/private_keyX.pem and client/public_keyX.pem where X is the local client number (e.g. testClientX) will regenerate a client's keys when running userClient.
- Run ```cp userClient userClientX.cpp``` to create another user client.
- You will need to create a new call in the Makefile for userClientX identical to userClient but replacing the dependency of userClient.cpp with userClientX.cpp.
  - There are many dependant files so it is important to modify the Makefile for new clients.
 
 # Additional Documentation
 Additional documentation can be found in client/ClientDocumentation.md and server-files/serverDocumentation.md.

 client/MessageGenerator.h contains comments about each MessageGenerator function and client/client_utilities.h contains comments about each ClientUtilities function.

 server-files/server_utilities.h contains comments about each ServerUtilities function.  

# Vulnerable Code

### Vulnerability #1
 In server.cpp, there is an insecure string copy which can cause a stack overflow, luckily it is protected by the compiler.... except that the developer disable stack protection and memory protection for debugging!!! Oh no!
    Insecure Copy /server.cpp Line 115
    server-debug Makefile Line 40
### Vulnerability #2
 In the gitignore, there is no ignorance of .pem files, the files used for key generation. This will likely lead to a user or users leaking keys at some point.
 This is a common way that users leak private information on the internet and has potential to cause issues later down the line as commit history cannot be removed.
### Vulnerability #3
 No input validation is being run for messages sent from a client to the server. This will allow buffer overflow vulnerabilities.
### Vulnerability #4
The server mapping JSON files are not cleared over time. When clients connect to a server, if it is their first time connecting, it assigns them an ID and adds their public key to a JSON file containing ID<->Public Key mappings. If the server were to take on enough clients and the JSON mapping file were to become large enough, when the server reads this file and converts it to a map, it would consume too much memory and drastically decrease the server performance or cause it to crash. 
