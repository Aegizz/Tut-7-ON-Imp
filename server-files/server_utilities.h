#ifndef server_utilities_h
#define server_utilities_h

#include "server_list.h"

#include <iostream>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <nlohmann/json.hpp> // For JSON library

// For client 
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <unordered_set>
#include <mutex>

#include "server_signature.h"
#include "server_key_gen.h"
#include "../client/Fingerprint.h"
#include "server_list.h"

struct deflate_config : public websocketpp::config::debug_core {
    typedef deflate_config type;
    typedef debug_core base;
    
    typedef base::concurrency_type concurrency_type;
    
    typedef base::request_type request_type;
    typedef base::response_type response_type;

    typedef base::message_type message_type;
    typedef base::con_msg_manager_type con_msg_manager_type;
    typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;
    
    typedef base::alog_type alog_type;
    typedef base::elog_type elog_type;
    
    typedef base::rng_type rng_type;
    
    struct transport_config : public base::transport_config {
        typedef type::concurrency_type concurrency_type;
        typedef type::alog_type alog_type;
        typedef type::elog_type elog_type;
        typedef type::request_type request_type;
        typedef type::response_type response_type;
        typedef websocketpp::transport::asio::basic_socket::endpoint 
            socket_type;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config> 
        transport_type;
        
    /// permessage_compress extension
    struct permessage_deflate_config {};

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;
};

typedef websocketpp::server<deflate_config> server;
typedef server::message_ptr message_ptr;

// WebSocket++ client configuration
typedef websocketpp::client<websocketpp::config::asio_client> client;

// Data structure to manage connections and their timers
struct connection_data{
    server* server_instance;
    client* client_instance;
    websocketpp::connection_hdl connection_hdl;
    server::timer_ptr timer;
    std::string server_address;
    int client_id;
    int server_id;
};

// Functions used to hash connection_hdl's
struct connection_hdl_hash {
    std::size_t operator()(websocketpp::connection_hdl hdl) const {
        return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(hdl.lock().get()));
    }
};
struct connection_hdl_equal {
    bool operator()(websocketpp::connection_hdl a, websocketpp::connection_hdl b) const {
        return a.lock() == b.lock();
    }
};



class ServerUtilities{
    private:
        // Stores the server's URI when instantiated as an object
        std::string myUri;
    public:
        ServerUtilities(const std::string uri);

        std::string getIP(server* s, websocketpp::connection_hdl hdl);

        bool is_connection_open(client* c, websocketpp::connection_hdl hdl);
        
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
        void connect_to_server(client* c, std::string const & uri, int server_id, EVP_PKEY* private_key, int counter, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal>* outbound_server_server_map, std::mutex* outbound_map_mutex, int retry_attempts = 0);

};

#endif