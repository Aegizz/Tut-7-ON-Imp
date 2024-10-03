# Server-Server Information

The following server-server featues have been implemented

- server_hello (unsigned so there is no server verification)
- client_update
- client_update_request
  
There exists server 1 hosted on ws://localhost:9002 and server 2 hosted on ws://localhost:9003.

When starting server 1, it continuously attempts to connect to server 2 and vice versa for when starting server 2.
It iterates through server_uris, an unordered_map<int, string> of servers uris stored against their ids.

Each server has its own client instance that connects and communicates with other servers on behalf of its master server.

### ServerList

It also instantiates a ServerList object which loads an existing mapping of client's IDs to their public keys from server_mapping'x'.json.
This file will be continuously updated as more clients connect to a server.
These currently contain the testClient's keys in them and can be modified however.

The ServerList is capable of producing client_list and client_update messages and manages the client list of the server, as well as a list of all the clients in the neighbourhood. 

### Connecting

When a connection is initiated with the server, it is unaware of whether it is a client connecting or a client connecting on behalf of a server (in other words a server). Initially it stores the connection_hdl in connection_map and starts a 10 second 
timer. If the timer expires, the connection is cancelled. 

Once a hello or server_hello message is received, it is able to fill out the details of the connection and determine if it is a client or server connecting. If it is a client connecting, it places the connection_data structure in client_server_map, and
if it is a server connecting, it places the connection_data structure in inbound_server_server_map. Both are stored against their connection_hdl.

Each server has a map for inbound connections (connections which it receives messages from other servers on) or outbound connections (connections which it sends messages to other servers on). In the current implementation, the server should not respond to
the other server over the inbound connection.

Once a client connection has been established, the client is added to the list and client lists are sent to the other clients connected to the home server and client updates are broadcasted to the neighbourhood. 

Once a server connection has been established, client updates are broadcasted to all existing servers except for the server which initiated the connection as it will momentarily make a client_update_request.

### Disconnecting

When a connection is closed with a server, it searches for a connection_data structure in client_server_map and inbound_server_server_map using the connection_hdl to determine if a client is closing their connection or a server.

If it is a client disconnecting, it removes the client from its client list and sends client lists to all clients except for the client that is disconnecting. It then erases the client from the client_server_map and sends client updates to the neighbourhood.

If it is a server disconnecting, it removes the server from its server list. It then finds the outbound connection to the disconnecting server and closes it if it is open. It then erases the connection from the inbound_server_server_map and sends client
lists to all of the clients connected to the server.

## Using the Program

Each server cpp file has a ServerID variable, a listenPort variable, and a myUri variable. ServerID specifies the ID of the server (which we assign), listenPort specifies the listenPort, 
and myUri is the uri of the websocket server (excluding the ws://). An unordered_map<int, string> server_uris exists which stores server IDs against their URIs.

server_list_h includes an unordered_map<int, string> serverAddresses which stores the neighbourhood of server IDs against their URIs.

When adding new servers to the neighbourhood, server_uris and serverAddresses will need to be updated.

All testClient files are currently have a string variable PUBLIC_KEY which are 6 capital letters.

### How I have been testing

My tests have been launching server then server2, then connecting the clients in any order, but mainly testClient, then testClient2, then testClient3. 

testClient connects to server 1, testClient2 connects to server 2, and testClient3 connects to server 1. All of them say hello then sleep for 60 seconds.
