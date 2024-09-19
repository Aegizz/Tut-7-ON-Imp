#include <iostream>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <nlohmann/json.hpp> // For JSON library

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
    // Create client list JSON object
    nlohmann::json clientList;

    // Set type
    clientList["type"] = "client_list";

    // Create array of JSON objects for all connected servers
    nlohmann::json servers = nlohmann::json::array();
    int numServers=1; // If there was 1 server (would be retrieved from external data structure)

    for(int i=0; i<numServers; i++){
        // Create JSON object for connected server
        nlohmann::json server;

        // Set fields
        server["address"] = "<Address of server>";
        server["server-id"] = "<server-id>";

        // Create array of JSON objects for clients connected to server
        nlohmann::json serverClients = nlohmann::json::array();
        int numClients=2; // If there were 2 clients stored in a client list (would be retrieved from external data structure)

        for(int j=0; j<numClients; j++){
            // Build client JSON object
            nlohmann::json clients;
            clients["client-id"] = "<client-id>";
            clients["public-key"] = "<Exported RSA public key of client>";
            
            // Push to array of clients of server
            serverClients.push_back(clients);
        }
        
        // Set clients field to array of clients in server
        server["clients"] = serverClients;

        // Push back array server to array of servers
        servers.push_back(server);   
    }
    // Add servers JSON array
    clientList["servers"] = servers;

    // Serialize JSON object
    std::string json_string = clientList.dump();

    try {
        s->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client list" << std::endl;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send client list because: " << e.what() << std::endl;
    }
}

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

    if(data["type"] == "hello"){
        // Store public key of new user
        std::string pubKey = data["public_key"];

        // Update client list
        // Send out client update to other servers (can worry about later)
    }else if(data["type"] == "client_list_request"){
        send_client_list(s, hdl);
    }

    /*try {
        s->send(hdl, buffer, msg->get_opcode());
        std::cout << "Sent echo message" << std::endl;
    } catch (const websocketpp::exception & e) {
        std::cout << "Echo failed because: " << e.what() << std::endl;
        return -1;
    }*/
    return 0;
}



int main(int argc, char * argv[]) {
    server echo_server;

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
        
        // Register our message handler
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
