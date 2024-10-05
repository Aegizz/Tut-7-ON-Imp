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

#include "server_signature.h"
#include "server_key_gen.h"

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
        std::string myUri;
    public:
        ServerUtilities(const std::string uri);

        std::string getIP(server* s, websocketpp::connection_hdl hdl);

        bool is_connection_open(client* c, websocketpp::connection_hdl hdl);
        int send_server_hello(client* c, websocketpp::connection_hdl hdl, EVP_PKEY* private_key, int counter);
        int send_client_update_request(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map);
        int send_client_update(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, ServerList* global_server_list);
        void broadcast_client_updates(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, ServerList* global_server_list, int server_id_nosend = 0);
        int send_client_list(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, ServerList* global_server_list);
        void broadcast_client_lists(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, ServerList* global_server_list, int client_id_nosend = 0);
        void connect_to_server(client* c, std::string const & uri, int server_id, EVP_PKEY* private_key, int counter, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal>* outbound_server_server_map, int retry_attempts = 0);

};

#endif