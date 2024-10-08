# Server Documentation

The following server featues have been implemented

- Signature verification
- server_hello
- client_update
- client_update_request
- client_list
- public and private chat forwarding
  
There exists server 1 hosted on ws://localhost:9002, server 2 hosted on ws://localhost:9003 and server 3 hosted on ws://localhost:9004.

When starting a server it continuously attempts to connect to the other servers for the number of attempts specified in the fail handler of the connect_to_server function in the ServerUtilities class. 
The server iterates through server_uris, an unordered_map<int, string> of servers uris stored against their serverIDs, which is retrieved from the ServerList class.

Each server is able to create client instances that connect and communicate with other servers on behalf of it's server.

## Connection Types
The server can have clients connect to it, have servers connect to it, and connect to other servers. Though messages could be sent over a single connection with another server, to simplify the implementation, a server can only communicate with another server over a connection they have initiated with that server . For this reason there exist three connection types:

- client_server connections: A connection between a client and a server.
- inbound_server_server connections: A connection another server has initiated. Messages are only received over this connection.
- outbound_server_server connections: A connection initiated by a server with another server. Messages are only sent over this connection.

## Current Implementation
The server currently tries to establish a connection to all the known servers, which at this stage are ws://localhost:9002, ws://localhost:9003, ws://localhost:9004.

Each server has a ServerID variable which is set to their server ID, a listenPort variable which is set to their listening port, and myAddress variable which is set to their address.

The server creates a ServerList object, providing the ServerID variable to create an object specifically for that server. It also creates a ServerUtilities class, providing their address to create an object specifically for that server.

## ServerList

This class stores the list of servers and their clients, clients connected to the server, and all clients native to the server that are known.

### Obtaining a server's public key
```
    EVP_PKEY* getPKey(int server_id);
    /*
        Iterate over knownServers map looking for server_id.
        If server_id was not found in the map, return nullptr.
        Convert string public key to PEM format.
        Return key.
    */   
```

### Obtaining a server's ID
```
    int ObtainID(std::string address);
    /*
        Iterate over defined serverAddresses map looking for specified address.
        Return ID when found.
        Otherwise return -1.
    */   
```

### Generating server_uris
```
    std::unordered_map<int, std::string> getUris();
    /*
        Iterate over a copy of serverAddress map defined in ServerList class.
        Add "ws://" to front of each address to create uri.
        Overwrite address for that server with created uri.
        Erase uri of the server calling the function.
        return the created map
    */   
```

### Retrieving Cliets
```
    std::unordered_map<int, std::string> getClients(int server_id);
    /*
        Create empty unordered_map<int, string>.
        If the server_id is contained in servers map, set that map equal to the 
        map of the clients for that server.
        Otherwise return the empty map.
    */   
```

### Adding a client to client list
```
    int insertClient(std::string public_key);
    /*
        Iterate over map of known clients and check if a previous ID exists. (client is known to server)
        Increment clientID value if not known before.
        Use client ID to add client to my_server in the server map.
        Generate fingerprint using public key and add to my_server in the serverFingerprints map, storing against fingerprint rather than ID.
        Use client ID to add client and their public key into currentClients map.
        Add client to map of known clients if they weren't previously known.
        Save knownClients map to a JSON file.
        return the generated client ID
    */
```

### Removing a client from client list
```
    void ServerList::removeClient(int client_id);
    /*
        Iterate through map of clients for my_server in servers map and find public_key.
        Iterate through map of clients for my_server in serverFingerprints map and erase client whose public key matches.
        Remove client from this server in servers map.
        Remove client from current connected clients map 
    */
```

## Server Connection Handlers

### When a connection is opened
```
    void on_open(server* s, websocketpp::connection_hdl hdl);
    /*
        Create connection data for incoming connection.
        Add server instance and connection handle to data structure.
        Start a 10 second timer and set the timer in the data structure. Close the connection once the timer has expired.
        Place the connection in the connection map (for temporary connections).
    */
```

### When a connection is closed
```
    void on_close(server* s, websocketpp::connection_hdl hdl);
    /*
        Check if the handle is in the client connections map or the inbound connections map.
        If in the client connection map
            Remove client from client list,
            Send updated client list to all clients except the client closing their connection.
            Erase the client from the client connections map.
            Send client updates to all servers.
        If in the inbound connections map
            Remove the server from the server list.
            Find an outbound connection to the closing server and close the connection if it is open.
            Erase the server from the inbound connections map.
            Send updated client list to all clients.
    */
```

### When a message is received
```
    int on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg);
    /*
        Parse the JSON message into a JSON object messageJSON.
        Check if the connection the message was received on is in the connection_map for unconfirmed connections or if it is in the inbound_server_server_map meaning it has been confirmed.
        If the message is a hello
            Cancel connection timer.
            Extract signature and counter from JSON object and convert public key from a string to a PEM.
            Verify the signature using the signature, data message, counter and public key.
            If the signature cannot be verified
                Close the connection with the client and remove from unconfirmed connections map.
            If the signature can be verified
                Insert client into client list, providing public key that was received in the message.
                Take the returned client ID and set it as the client ID in the connection structure.
                Move the connection to the client connection map.
                Send updated client list to all clients except the newly established one.
                Send client updates to all servers.
        If the message is a server hello
            Cancel connection timer.
            Set the address sent in the message as the server address in the connection structure and use the address to obtain the server ID from the ServerList object.
            If an invalid address was entered, return an error.
            Obtain server's public key using server ID.
            Attempt to verify the signature.
            If the signature cannot be verified
                Close the connection with the server and remove from unconfirmed connections map.
            If the signature can be verified
                Iterate over inbound connections map and find if an inbound connection already exists to this server.
                    If a connection exists, close the new connection, erase it from the temporary connection map and return an error.
                    Otherwise do nothing.
                    Add connection data to inbound connections map and erase from temporary connections map.
                    Check if an outbound connection exists, and if it doesn't attempt to establish the connection.
                    Send client updates to all servers.
        If the message is a public chat
            Extract signature and counter.
            If the connection is an inbound connection (so message has been forwarded)
                Obtain serverID from connection_data structure.
                Obtain public key for sender using fingerprint and serverID
                Attempt to verify the signature of the sender
                    If the signature cannot be verified
                        Do not forward the message and return an error
                    If the signature can be verified
                        Broadcast the public chats to all clients connected to the server.
            If the connection is a client connection (so message needs to be sent out)
                Set serverID as this server.
                Obtain public key for sender using fingerprint and serverID
                Attempt to verify the signature of the sender
                    If the signature cannot be verified
                        Do not forward the message and return an error.
                    If the signature can be verified
                        Broadcast the public chats to all clients connected to the server except the sender.
                        Broadcast the public chat to all servers.
        If the message is a private chat
            Extract the signature and counter from the message for signature verification.
            If the connection is an inbound connection (so message has been forwarded)
                Broadcast the private chat to all clients on the server.
            If the connection is a client connection (so message needs to be sent out)
                Obtain the client ID from the server.
                Obtain the client's public key from ServerList and convert it to PEM.
                Attempt to verify the signature
                    If the signature cannot be verified
                        Do not forward the message and return an error.
                    If the signature can be verified
                        Create a set of the destination_servers (so every address is unique).
                        Broadcast private chat to clients if this server is the home server of a recipient.
                        Broadcast private chat to all servers in the set
        If the message is a client list request
            Send the client list on the connection to the requesting client.
        If the message is a client update request
            Find the outbound connection for the requesting server and send the client update on that connection.
        If the message is a client update
            Use the insertServer function in the ServerList object to process the client update.
            Broadcast client lists to all servers.
    */
```


## Connecting to a server
```
    // Using the ServerUtilities object
    void connect_to_server(client* c, std::string const & uri, int server_id, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal>* outbound_server_server_map, int retry_attempts = 0);
    /*
        Check if an outbound connection exists, and prematurely return if it does.
        Attempt to make a connection using the provided uri.
        If the connection fails
            Pause the client thread for 500ms and then try to make another connection, incrementing the retry_attempts variable.
            Stop trying to connect once retry_attempts is equal to the specified number of tries.
        If the connection succeeds
            Send a server hello to verify the message.
            Create and fill out a connection data structure and add the connection data to the outbound connections map.
            Send a client update request to the server a connection was just established with.
        If the connection closes
            Erase the connection from the outbound connections map.
            Attempt to reconnect to the server using the provided uri.
    */
```
