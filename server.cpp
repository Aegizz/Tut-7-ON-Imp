#include <iostream>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <nlohmann/json.hpp> // For JSON library

//Self made client list implementation
#include "server-files/server_list.h"

//Global pointer for client list
ServerList * global_server_list = nullptr;

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

void send_client_list(server* s, websocketpp::connection_hdl hdl){
    std::string json_string = global_server_list->exportClientList();

    try {
        s->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client list" << std::endl;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send client list because: " << e.what() << std::endl;
    }
}

// Data structure to manage connections and their timers
struct connection_data{
    server* server_instance;
    websocketpp::connection_hdl connection_hdl;
    server::timer_ptr timer;
    int client_id;
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

// Define connection map
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> connection_map; 

// Define a callback to handle incoming messages
int on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;

    // Vulnerable code: the payload without validation
    std::string payload = msg->get_payload();

    char buffer[1024];
    // Potential buffer overflow: Copying payload directly into a fixed-size buffer
    strcpy(buffer, payload.c_str()); // This is unsafe if payload length exceeds buffer size

    // Deserialize JSON message
    nlohmann::json data = nlohmann::json::parse(payload);

    auto con_data = connection_map[hdl];
    if(data["data"]["type"] == "hello"){
        
        // Cancel connection timer
        con_data->timer->cancel();
        std::cout << "Cancelling timer" << std::endl;

        // Update client list, this is server 1 (we will manually assign server ids)
        con_data->client_id = global_server_list->insertClient(data["data"]["public_key"]);

        // Send out client update to other servers
    }else if(data["type"] == "client_list_request"){
        send_client_list(s, hdl);
    }
    
    return 0;
}

std::string getClientIP(server* s, websocketpp::connection_hdl hdl){
    // Get the connection pointer
    server::connection_ptr con = s->get_con_from_hdl(hdl);

    // Get the remote endpoint (IP address and port)
    std::string remote_endpoint = con->get_remote_endpoint();

    // Find start and end of IP
    size_t start = remote_endpoint.find_last_of("f")+2;
    size_t end = remote_endpoint.find_last_of("]");

    // Generate substring
    std::string IP = remote_endpoint.substr(start, end-start);

    // If we need to use port
    start = remote_endpoint.find_last_of(":");

    std::string port = remote_endpoint.substr(start,remote_endpoint.size()-start);

    std::string IPPort = IP.append(port);

    return IPPort;
}

void on_close(server* s, websocketpp::connection_hdl hdl){
    std::cout << "Connection closed from: " << getClientIP(s, hdl) << std::endl;

    // Grab ID and remove from connection map
    auto con_data = connection_map[hdl];
    global_server_list->removeClient(con_data->client_id);

    // Send out client_lists
    // Send out client_update
}

void on_open(server* s, websocketpp::connection_hdl hdl){
    std::cout << "Connection initiated from: " << getClientIP(s, hdl) << std::endl;

    // Create shared connection_data structure and fill in
    auto con_data = std::make_shared<connection_data>();
    con_data->server_instance = s;
    con_data->connection_hdl = hdl;

    con_data->timer = s->set_timer(10000, [con_data](websocketpp::lib::error_code const &ec){
        if(ec){
            return;
        }
        // If timer runs out, close connection and remove from connection map
        std::cout << "Timer expired, closing connection." << std::endl;
        con_data->server_instance->close(con_data->connection_hdl, websocketpp::close::status::normal, "Hello not received from client.");
        connection_map.erase(con_data->connection_hdl);
    });
    // Place connection_data structure in map
    connection_map[hdl] = con_data;

    std::cout << "Connection confirmed from: " << getClientIP(s, hdl) << std::endl;
}



int main(int argc, char * argv[]) {
    server echo_server;

    // Initialise server list as server 1
    global_server_list = new ServerList(1);

    try {
        // Set logging settings        
        if (argc > 1 && std::string(argv[1]) == "-d") {
            echo_server.set_access_channels(websocketpp::log::alevel::all);
            echo_server.set_error_channels(websocketpp::log::elevel::all);
        } else {
            echo_server.set_access_channels(websocketpp::log::alevel::none);
            echo_server.set_error_channels(websocketpp::log::elevel::none);
        }

        // Initialize ASIO
        echo_server.init_asio();

        // Set handlers
        echo_server.set_open_handler(bind(&on_open, &echo_server, std::placeholders::_1));
        echo_server.set_close_handler(bind(&on_close, &echo_server, std::placeholders::_1));
        echo_server.set_message_handler(bind(&on_message, &echo_server, std::placeholders::_1, std::placeholders::_2));
        
        // Listen on port 9002
        echo_server.listen(9002);
        
        // Start the server accept loop
        echo_server.start_accept();
        
        // Start the ASIO io_service run loop
        echo_server.run();

    } catch (const websocketpp::exception & e) {
        std::cout << "WebSocket++ exception: " << e.what() << std::endl;
    } catch (const std::exception & e) {
        std::cout << "Standard exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception" << std::endl;
    }
}
