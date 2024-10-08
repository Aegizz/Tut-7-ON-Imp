# Server Documentation

The following server featues have been implemented

- server_hello (unsigned so there is no server verification yet)
- client_update
- client_update_request
- client_list
  
There exists server 1 hosted on ws://localhost:9002, server 2 hosted on ws://localhost:9003 and server 3 hosted on ws://localhost:9004.

When starting a server it continuously attempts to connect to the other servers for the number of attempts specified in the fail handler of the connect_to_server function in the ServerUtilities class. 
The server iterates through server_uris, an unordered_map<int, string> of servers uris stored against their serverIDs, which is retrieved from the ServerList class.

Each server is able to create client instances that connect and communicate with other servers on behalf of it's server.

## Connection Types
The server can have clients connect to it, have servers connect to it, and connect to other servers. Though messages could be sent over a connection with another server, to simplify the implementation, a server can only communicate with another server over a connection they have established with that server. For this reason there exist three connection types:

- client_server connections: A connection between a client and a server.
- inbound_server_server connections: A connection another server has established. Messages are only received over this connection.
- outbound_server_server connections: A connection established by a server with another server. Messages are only sent over this connection.

## ServerList

This class stores the list of servers and their clients, clients connected to the server, and all clients native to the server that are known.

```
    /*
        Retrives a Server's public key using their server ID.

        int server_id - ID of a server
    */
    EVP_PKEY* getPKey(int server_id);
        Iterate over knownServers map looking for server_id.
        If server_id was not found in the map, return nullptr.
        Convert string public key to PEM format.
        Return key.

    /*
        Retrives a Server's ID using an address.

        std::string address - Server address
    */
    int ObtainID(std::string address);
        Iterate over defined serverAddresses map looking for specified address.
        Return ID when found.
        Otherwise return -1.

    /*
        Generates URIs of all other servers.
    */
    std::unordered_map<int, std::string> getUris();
        Iterate over a copy of serverAddress map defined in ServerList class.
        Add "ws://" to front of each address to create uri.
        Overwrite address for that server with created uri.
        Erase uri of the server calling the function.
        return the created map

    /*
        Retrieves map of clients with their ID stored against their public keys.

        int server_id - ID of a server
    */
    std::unordered_map<int, std::string> getClients(int server_id);
        Create empty unordered_map<int, string>.
        If the server_id is contained in servers map, set that map equal to the 
        map of the clients for that server.
        Otherwise return the empty map.

    /*
        Adds a client to the client list and generates and stores their fingerprint.

        std::string public_key - Public key of a client
    */
    int insertClient(std::string public_key);
        Iterate over map of known clients and check if a previous ID exists. (client is known to server)
        Increment clientID value if not known before.
        Use client ID to add client and their public key into currentClients map.
        Add client to map of known clients if they weren't previously known.
        Save knownClients map to a JSON file.
        return the generated client ID

    /*
        Removes a client from the client list and removes their fingerprints.

        std::string public_key - ID of a client
    */
    void ServerList::removeClient(int client_id);
    /*
        Remove client from this server in servers map.
        Remove client from current connected clients map 
    */
```


## Server Utilities
```
    /*
        Server_Hello
        This message is sent when a server first connects to another server.
        {
            "data": {
                "type": "server_hello",
                "sender": "<server IP connecting>"
            }
        }

        This message is always signed.
        {
            "type": "signed_data",
            "data": {  },
            "counter": 12345,
            "signature": "<Base64 signature of data + counter>"
        }

        client* c - Client instance of server-server connection
        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        EVP_PKEY* private_key - Private key of server
        int counter - Current counter value

    */
    int send_server_hello(client* c, websocketpp::connection_hdl hdl, EVP_PKEY* private_key, int counter);

    /*
        Client Update Request
        To retrieve a list of all currently connected clients on a server. The server will send a JSON response.
        This message is sent to all servers that a connection is established with.

        {
            "type": "client_update_request"
        }
        This is NOT signed and does NOT follow the data format.

        client* c - Client instance of server-server connection
        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections 
    */
    int send_client_update_request(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map);
    
    /*
        Client Update
        To communicate which clients are connected to a server.
        Note: Our modified implementation includes client ID for each client in the packet.

        {
            "type": "client_update",
            "clients": [
                {
                    "client-id":"<client-id>",
                    "public-key":"<public-key>"
                },
            ]
        }
        This is NOT signed and does NOT follow the data format.

        client* c - Client instance of server-server connection
        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections
        ServerList* global_server_list - Pointer to server's ServerList object to generate client update JSON
    */
    int send_client_update(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, ServerList* global_server_list);
    
    /*
        Calls send_client_update() function for all servers except the one specified (if provided in call).

        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections
        ServerList* global_server_list - Pointer to server's ServerList object to generate client update JSON
        int server_id_nosend - Server ID of server to not send client update to
    */
    void broadcast_client_updates(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, ServerList* global_server_list, int server_id_nosend = 0);
    
    /*
        Client list
        To communicate to clients which clients are connected on all connected servers.
        Note: Our modified implementation includes server-id for each server and client-id for each client in the packet.

        {
            "type": "client_list",
            "servers": [
                {
                    "address": "<Address of server>",
                    "server-id":"<server-id>",
                    "clients": [
                        {
                            "client-id":"<client-id>",
                            "public-key":"<Exported RSA public key of client>"

                        },
                    ]
                },
            ]
        }
        This is NOT signed and does NOT follow the data format.

        server* s - Server instance of client-server connection
        websocketpp::connection_hdl hdl - Connection handle of client-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        client_server_map - Map of client-server connections
        ServerList* global_server_list - Pointer to server's ServerList object to generate client list JSON
    */
    int send_client_list(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, ServerList* global_server_list);

    /*
        Calls send_client_list() function for all clients except the one specified (if provided in call).

        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        client_server_map - Map of client-server connections
        ServerList* global_server_list - Pointer to server's ServerList object to generate client list JSON
        int client_id_nosend - Client ID of client to not send client list to
    */
    void broadcast_client_lists(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, ServerList* global_server_list, int client_id_nosend = 0);

    /*
        Public Chat Forwarding to Servers

        This function does not formulate any public chat messages, it is only responsible for forwarding them on to another server.

        client* c - Client instance of server-server connection
        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections
        std::string message - String form of JSON signed public chat message
    */
    int send_public_chat_server(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message);
    
    /*
        Calls send_public_chat_server() function for all servers except the one specified.

        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections
        std::string message - String form of JSON signed public chat message
        int server_id_nosend - Server ID of server to not send public chat to
    */
    void broadcast_public_chat_servers(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message, int server_id_nosend);
    
    /*
        Public Chat Forwarding to Clients

        This function does not formulate any public chat messages, it is only responsible for forwarding them on to another client.

        server* s - Server instance of client-server connection
        websocketpp::connection_hdl hdl - Connection handle of client-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        client_server_map - Map of client-server connections
        std::string message - String form of JSON signed public chat message
    */
    int send_public_chat_client(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message);
    
    /*
        Calls send_public_chat_client() function for all clients except the one specified (if provided in call).

        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        client_server_map - Map of client-server connections
        std::string message - String form of JSON signed public chat message
        int client_id_nosend - Client ID of client to not send public chat to
    */
    void broadcast_public_chat_clients(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message, int client_id_nosend=0);

    /*
        Private Chat Forwarding to Servers

        This function does not formulate any private chat messages, it is only responsible for forwarding them on to another server.

        client* c - Client instance of server-server connection
        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections
        std::string message - String form of JSON signed private chat message
    */
    int send_private_chat_server(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message);
    
    /*
        Calls send_private_chat_server() function for all servers.

        std::unordered_set<std::string> serverSet - Set of servers to forward the private chat to
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Map of outbound connections
        std::string message - String form of JSON signed private chat message
    */
    void broadcast_private_chat_servers(std::unordered_set<std::string> serverSet, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message);

    /*
        Private Chat Forwarding to Clients

        This function does not formulate any private chat messages, it is only responsible for forwarding them on to another client.

        server* s - Server instance of client-server connection
        websocketpp::connection_hdl hdl - Connection handle of client-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        client_server_map - Map of client-server connections
        std::string message - String form of JSON signed private chat message
    */
    int send_private_chat_client(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message);
    
    /*
        Calls send_private_chat_client() function for all clients.

        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        client_server_map - Map of client-server connections
        std::string message - String form of JSON signed private chat message
        int client_id_nosend - Client ID of client to not send private chat to
    */
    void broadcast_private_chat_clients(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message, int client_id_nosend=0);

    /*
        Creates connections to other servers (outbound connections).
        Responsible for providing data to create and manage outbound connections.

        client* c - Client instance for server-server connection
        std::string const & uri - URI of server to connect to
        int server_id - ID of server to connect to
        EVP_PKEY* private_key - Private key of this server
        int counter - Current counter value stored on server
        websocketpp::connection_hdl hdl - Connection handle of server-server connection
        std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> 
        outbound_server_server_map - Pointer to map of outbound connections (need to add created connections to the map)
    */
    void connect_to_server(client* c, std::string const & uri, int server_id, EVP_PKEY* private_key, int counter, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal>* outbound_server_server_map, int retry_attempts = 0);
```

## Server Connection Handlers
```
    /*
        When a connection is opened with the server
    */
    void on_open(server* s, websocketpp::connection_hdl hdl);
    /*
        Create connection data for incoming connection.
        Add server instance and connection handle to data structure.
        Start a 10 second timer and set the timer in the data structure. Close the connection once the timer has expired.
        Place the connection in the connection map (for temporary connections).
    */

    /*
        When a connection is closed with the server
    */
    void on_close(server* s, websocketpp::connection_hdl hdl);
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

    /*
        When a connection is received by the server
    */
    int on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg);
    /*
        Parse the JSON message into a JSON object.
        Check if the connection the message was received on has been confirmed, or if is a confirmed connection (an inbound connection).
        If the message is a hello
            Cancel connection timer.
            Insert client into client list, providing public key that was received in the message.
            Take the returned client ID and set it as the client ID in the connection structure.
            Move the connection to the client connection map.
            Send updated client list to all clients except the newly established one.
            Send client updates to all servers.
        If the message is a server hello
            Cancel connection timer.
            Set the address sent in the message as the server address in the connection structure and use the address to obtain the server ID from the ServerList object.
            Iterate over inbound connections map and find if an inbound connection already exists to this server.
                If a connectione exists, close the new connection, erase it from the temporary connection map and return an error.
                Otherwise do nothing.
            Add connection data to inbound connections map and erase from temporary connections map.
            Check if an outbound connection exists, and if it doesn't attempt to establish the connection.
            Send client updates to all servers.
        If the message is a client list request
            Send the client list on the connection to the requesting client.
        If the message is a client update request
            Find the outbound connection for the requesting server and send the client update on that connection.
        If the message is a client update
            Use the insertServer function in the ServerList object to process the client update.
            Broadcast client lists to all servers.
    */
```
