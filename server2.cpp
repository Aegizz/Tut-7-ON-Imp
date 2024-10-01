#include <iostream>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <nlohmann/json.hpp> // For JSON library

// For client 
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <chrono>
#include <thread>
#include <functional>
#include <vector>

//Self made client list implementation
#include "server-files/server_list.h"

// Hard coded server ID + listen port for this server
const int ServerID = 2; 
const int listenPort = 9003;

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

// Define connection map
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> connection_map; 
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map;

// Map for connections made from other servers -> this server
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> inbound_server_server_map;

// Map for connections made from this server -> other servers
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map;

// Find IP address + port number of connected client
std::string getIP(server* s, websocketpp::connection_hdl hdl){
    // Get the remote endpoint (IP address and port)
    std::string remote_endpoint;

    // Get the connection pointer
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    // Get the remote endpoint (IP address and port)
    remote_endpoint = con->get_remote_endpoint();

    // Handle error case when connections have been closed
    if(remote_endpoint == "Unknown"){
        return remote_endpoint;
    }

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

// Check if outbound connection is open
bool is_connection_open(client* c, websocketpp::connection_hdl hdl){
    client::connection_ptr con = c->get_con_from_hdl(hdl);

    if(con->get_state() == websocketpp::session::state::open){
        return true;
    }
    return false;
}

void send_server_hello(client* c, websocketpp::connection_hdl hdl){
    nlohmann::json data;

    // Format server_hello message
    data["type"] = "server_hello";
    data["sender"] = "127.0.0.1:9003";
    data["server_id"] = ServerID;

    nlohmann::json serverHello;

    serverHello["data"] = data;

    // Serialize JSON object
    std::string json_string = serverHello.dump();

    // Send the message via the connection
    if(!is_connection_open(c, hdl)){
        return;
    }
    websocketpp::lib::error_code ec;
    c->send(hdl, json_string, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cout << "> Error sending server hello message: " << ec.message() << std::endl;
    } else {
        std::cout << "> Server hello sent" << "\n" << std::endl;
    }
}

// Send client update request to specified connection
void send_client_update_request(client* c, websocketpp::connection_hdl hdl){
    nlohmann::json request;
    request["type"] = "client_update_request";

    // Serialize JSON object
    std::string json_string = request.dump();

    if(!is_connection_open(c, hdl)){
        return;
    }

    try {
        c->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client update request to " << outbound_server_server_map[hdl]->server_id << std::endl;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send update request to " << outbound_server_server_map[hdl]->server_id << " because: " << e.what() << std::endl;
    }
}

// Send client update to specified connection
void send_client_update(client* c, websocketpp::connection_hdl hdl){
    std::string json_string = global_server_list->exportUpdate();

    if(!is_connection_open(c, hdl)){
        return;
    }

    try {
        c->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client update to " << outbound_server_server_map[hdl]->server_id << std::endl;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send client update to " << outbound_server_server_map[hdl]->server_id << " because: " << e.what() << std::endl;
    }
}

// Send client updates to all servers but the one specified (if specified)
void broadcast_client_updates(int server_id_nosend = 0){
    // Broadcast client_updates
    for(const auto& connectPair: outbound_server_server_map){
        auto connection = connectPair.second;
        if(connection->server_id != server_id_nosend){
            send_client_update(connection->client_instance, connection->connection_hdl);
        }
    }
}

// Send client list to specified connection
void send_client_list(server* s, websocketpp::connection_hdl hdl){
    std::string json_string = global_server_list->exportClientList();

    try {
        s->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client list to client " << client_server_map[hdl]->client_id <<  std::endl;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send client list because: " << e.what() << std::endl;
    }
}

// Handle incoming connections
void on_open(server* s, websocketpp::connection_hdl hdl){
    std::cout << "\nConnection initiated from: " << getIP(s, hdl) << std::endl;

    // Create shared connection_data structure and fill in
    auto con_data = std::make_shared<connection_data>();
    con_data->server_instance = s;
    con_data->connection_hdl = hdl;

    // Create and set timer for connection
    con_data->timer = s->set_timer(10000, [con_data](websocketpp::lib::error_code const &ec){
        if(ec){
            return;
        }
        // If timer runs out, close connection and remove from connection map
        std::cout << "Timer expired, closing connection." << std::endl;
        con_data->server_instance->close(con_data->connection_hdl, websocketpp::close::status::normal, "Hello not received from client.");
    });
    // Place connection_data structure in map
    connection_map[hdl] = con_data;
}

// Handle closing connections
void on_close(server* s, websocketpp::connection_hdl hdl){
    // Create iterators to check if the connection being closed is a client or inbound server connection
    auto it_client = client_server_map.find(hdl);
    auto it_server = inbound_server_server_map.find(hdl);

    // If the connection being closed is a client connection
    if (it_client != client_server_map.end()) {
        // Client connection
        std::cout << "\nClient " << client_server_map[hdl]->client_id << " closing their connection" << std::endl;
        global_server_list->removeClient(client_server_map[hdl]->client_id);

        // Send out client_lists to everyone but the closing connection
        for(const auto& connectPair: client_server_map){
            auto connection = connectPair.second;
            if(connection->client_id != client_server_map[hdl]->client_id){
                send_client_list(connection->server_instance, connection->connection_hdl);
            }
        }

        // Erase from client server map
        if(client_server_map.find(hdl) != client_server_map.end()){
            client_server_map.erase(hdl);
        }

        broadcast_client_updates();

    // If the connection being closed is an inbound server connection
    } else if (it_server != inbound_server_server_map.end()) {
        // Server connection
        std::cout << "\nServer " << inbound_server_server_map[hdl]->server_id << " closing inbound connection" << std::endl;
        global_server_list->removeServer(inbound_server_server_map[hdl]->server_id);

        // Close outbound connection
        for(const auto& connectPair: outbound_server_server_map){
            auto connection = connectPair.second;
            if(connection->server_id == inbound_server_server_map[hdl]->server_id){
                if(is_connection_open(connection->client_instance, connection->connection_hdl)){
                    connection->client_instance->close(connection->connection_hdl, websocketpp::close::status::normal, "Closing both connections");
                }
            }
        }

        // Erase from inbound connection map
        if(inbound_server_server_map.find(hdl) != inbound_server_server_map.end()){
            inbound_server_server_map.erase(hdl);
        }

        // Send out client_lists to all clients
        for(const auto& connectPair: client_server_map){
            auto connection = connectPair.second;
            send_client_list(connection->server_instance, connection->connection_hdl);
        }
    }
}

// Handle messages received by server
int on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;

    // Vulnerable code: the payload without validation
    std::string payload = msg->get_payload();

    char buffer[1024];
    // Potential buffer overflow: Copying payload directly into a fixed-size buffer
    strcpy(buffer, payload.c_str()); // This is unsafe if payload length exceeds buffer size

    // Deserialize JSON message
    nlohmann::json data = nlohmann::json::parse(payload);

    std::shared_ptr<connection_data> con_data;

    // Use handle to check if connection has been confirmed or not
    if(connection_map.find(hdl) != connection_map.end()){
        con_data = connection_map[hdl];
    }else if(inbound_server_server_map.find(hdl) != inbound_server_server_map.end()){
        con_data = inbound_server_server_map[hdl];
    }

    if(data["data"]["type"] == "hello"){
        // Cancel connection timer
        con_data->timer->cancel();
        std::cout << "Cancelling client connection timer" << std::endl;

        // Update client list
        con_data->client_id = global_server_list->insertClient(data["data"]["public_key"]);

        // Move to client server map
        client_server_map[hdl] = con_data;
        if(connection_map.find(hdl) != connection_map.end()){
            connection_map.erase(hdl);
        }

        // Send out client_lists to all other clients
        for(const auto& connectPair: client_server_map){
            auto connection = connectPair.second;
            if(connection->client_id != con_data->client_id){
                send_client_list(connection->server_instance, connection->connection_hdl);
            }
        }

        broadcast_client_updates();
    }else if(data["data"]["type"] == "server_hello"){
        // Cancel connection timer
        con_data->timer->cancel();
        std::cout << "Cancelling server connection timer" << std::endl;

        con_data->server_address = data["data"]["sender"];
        con_data->server_id = data["data"]["server_id"];

        // Check if an existing connection exists
        for(const auto& connectPair: inbound_server_server_map){
            auto connection = connectPair.second;
            if(connection->server_id == con_data->server_id){
                s->close(hdl, websocketpp::close::status::policy_violation, "Connection to this server already exists.");
                if(connection_map.find(hdl) != connection_map.end()){
                    connection_map.erase(hdl);
                }
                return -1;
            }
        }

        // Add connection data to map
        inbound_server_server_map[hdl] = con_data;

        // Erase from temporary connection map
        if(connection_map.find(hdl) != connection_map.end()){
            connection_map.erase(hdl);
        }

        // Broadcast client updates to all servers except connecting server
        broadcast_client_updates(con_data->server_id);

        // NOT SURE WHETHER TO IMPLEMENT
        /*
        // Check if an outbound connection has been established for the connecting server
        for(const auto& connectPair: outbound_server_server_map){
            auto connection = connectPair.second;
            // Hard coded as 1 because this is server 1
            if(connection->server_id == con_data->server_id){
                return 0;
            }
        }
        connect_to_server(con_data->client_instance, con_data->server_address, con_data->server_id);
        */

    }else if(data["type"] == "client_list_request"){
        // Send client list to requesting client
        send_client_list(s, hdl);
    }else if(data["type"] == "client_update_request"){
        // Find requesting server's outbound connection and send client update on that
        for(const auto& connectPair: outbound_server_server_map){
            auto connection = connectPair.second;
            
            if(connection->server_id == con_data->server_id){
                send_client_update(connection->client_instance, connection->connection_hdl);
            }
        }
    }else if(data["type"] == "client_update"){
        // Process client update
        global_server_list->insertServer(con_data->server_id, payload);

        // Send out client_lists to all clients
        for(const auto& connectPair: client_server_map){
            auto connection = connectPair.second;
            send_client_list(connection->server_instance, connection->connection_hdl);
        }
    }

    std::cout << "\n";

    return 0;
}



// Define a function that will handle the client connections retry logic
void connect_to_server(client* c, std::string const & uri, int server_id, int retry_attempts = 0) {
    websocketpp::lib::error_code ec;
    client::connection_ptr con = c->get_connection(uri, ec);

    if (ec) {
        std::cout << "Could not create connection because: " << ec.message() << std::endl;
        return;
    }

    // Set fail handler to retry if connection fails
    con->set_fail_handler([c, uri, server_id, retry_attempts](websocketpp::connection_hdl hdl) {
        std::cout << "Connection to " << uri << " failed, retrying in 500ms..." << std::endl;

        // Retry after 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (retry_attempts < 1000) { 
            connect_to_server(c, uri, server_id, retry_attempts + 1);
        } else {
            std::cout << "Exceeded retry limit. Giving up on connecting to " << uri << std::endl;
        }
    });

    // Set open handler when connection succeeds
    con->set_open_handler([c, uri, server_id](websocketpp::connection_hdl hdl) {
        std::cout << "\nSuccessfully connected to " << uri << std::endl;

        send_server_hello(c, hdl);
        
        auto con_data = std::make_shared<connection_data>();
        con_data->client_instance = c;
        con_data->connection_hdl = hdl;
        //con_data->server_address = IP;

        // EXPERIMENTAL PARAMETER
        con_data->server_id = server_id;

        // Add connection data to outbound connection map
        outbound_server_server_map[hdl] = con_data;

        // Has to be here otherwise a segmentation fault occurs
        send_client_update_request(c, hdl);
    });

    // Handler for when another server closes connection
    con->set_close_handler([](websocketpp::connection_hdl hdl) {
        std::cout << "\nServer " << outbound_server_server_map[hdl]->server_id << " closing outbound connection" << std::endl;

        // Erase connection from outbound connection map
        if(outbound_server_server_map.find(hdl) != outbound_server_server_map.end()){
            outbound_server_server_map.erase(hdl);
        }
    });

    // Try to connect to the server
    c->connect(con);
}



// Define a map to hold server uris against their IDs
// Different for each server file
std::unordered_map<int, std::string> server_uris = {{1, "ws://127.0.0.1:9002"}};

int main(int argc, char * argv[]) {
    // Create a WebSocket++ client instance
    // Global so server can initiate a connection to another server at any point
    client ws_client;

    server echo_server;

    // Initialise server list as server 2
    global_server_list = new ServerList(ServerID);

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

        // Start a separate thread to handle the client that connects to ws://localhost:9003
        std::thread client_thread([]() {
            std::cout << "Starting client thread..." << "\n" << std::endl;

            client ws_client;

            // Set logging settings for the client
            ws_client.set_access_channels(websocketpp::log::alevel::none);
            ws_client.set_error_channels(websocketpp::log::elevel::none);

            // Initialize ASIO for the client
            ws_client.init_asio();

            // Loop through the server URIs and attempt connections
            for(const auto& uri: server_uris){
                connect_to_server(&ws_client, uri.second, uri.first);

            }

            // Start the client io_service run loop
            ws_client.run();
        });

        // Detach the client thread so it runs independently
        client_thread.detach();
        
        // Listen on port 9003
        echo_server.listen(listenPort);
        
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
