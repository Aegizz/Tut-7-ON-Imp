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

//Include server files
#include "server-files/server_list.h"
#include "server-files/server_utilities.h"
#include "server-files/server_key_gen.h"
#include "server-files/server_signature.h"

// Hard coded server ID + listen port for this server
const int ServerID = 1; 
const int listenPort = 9002;
const std::string myAddress = "127.0.0.1:9002";

// Create key file names
std::string privFileName = "server-files/private_key_server" + std::to_string(ServerID) + ".pem";
std::string pubFileName = "server-files/public_key_server" + std::to_string(ServerID) + ".pem";

// Load private keys
EVP_PKEY* privKey;
EVP_PKEY* pubKey;

// Initialise global server list pointer as server 1
ServerList* global_server_list = new ServerList(ServerID);

ServerUtilities* serverUtilities = new ServerUtilities(myAddress);

// Obtain server uri map from server list object
// Will never contain this server in the map
std::unordered_map<int, std::string> server_uris = global_server_list->getUris();

typedef websocketpp::server<deflate_config> server;
typedef server::message_ptr message_ptr;

// WebSocket++ client configuration
typedef websocketpp::client<websocketpp::config::asio_client> client;


// Define connection map
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> connection_map; 
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map;

// Map for connections made from other servers -> this server
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> inbound_server_server_map;

// Map for connections made from this server -> other servers
std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map;


// Handle incoming connections
void on_open(server* s, websocketpp::connection_hdl hdl){
    std::cout << "\nConnection initiated from: " << serverUtilities->getIP(s, hdl) << std::endl;

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
        serverUtilities->broadcast_client_lists(client_server_map, global_server_list, client_server_map[hdl]->client_id);

        // Erase from client server map
        if(client_server_map.find(hdl) != client_server_map.end()){
            client_server_map.erase(hdl);
        }

        serverUtilities->broadcast_client_updates(outbound_server_server_map, global_server_list);

    // If the connection being closed is an inbound server connection
    } else if (it_server != inbound_server_server_map.end()) {
        // Server connection
        std::cout << "\nServer " << inbound_server_server_map[hdl]->server_id << " closing inbound connection" << std::endl;
        global_server_list->removeServer(inbound_server_server_map[hdl]->server_id);

        // Close outbound connection
        for(const auto& connectPair: outbound_server_server_map){
            auto connection = connectPair.second;
            if(connection->server_id == inbound_server_server_map[hdl]->server_id){
                if(serverUtilities->is_connection_open(connection->client_instance, connection->connection_hdl)){
                    connection->client_instance->close(connection->connection_hdl, websocketpp::close::status::normal, "Closing both connections");
                }
            }
        }

        // Erase from inbound connection map
        if(inbound_server_server_map.find(hdl) != inbound_server_server_map.end()){
            inbound_server_server_map.erase(hdl);
        }

        // Send out client_lists to all clients
        serverUtilities->broadcast_client_lists(client_server_map, global_server_list);
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
    }else if(client_server_map.find(hdl) == client_server_map.end()){
        std::cout << "Connection lost by server" << std::endl;
        return -1;
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
        serverUtilities->broadcast_client_lists(client_server_map, global_server_list, con_data->client_id);

        serverUtilities->broadcast_client_updates(outbound_server_server_map, global_server_list);
    }else if(data["data"]["type"] == "server_hello"){
        // Cancel connection timer
        con_data->timer->cancel();
        std::cout << "Cancelling server connection timer" << std::endl;

        con_data->server_address = data["data"]["sender"];
        //con_data->server_id = data["data"]["server_id"];

        // Need to add error handling in case foreign or invalid server address is entered
        std::string server_signature = data["signature"];

        con_data->server_id = global_server_list->ObtainID(con_data->server_address);

        // Obtain Public server's public key from mapping
        EVP_PKEY* serverPKey = global_server_list->getPKey(con_data->server_id);

        // Extract counter
        int counter = data["counter"];

        // Verify signature and close connection if invalid
        if(!ServerSignature::verifySignature(server_signature, data["data"].dump() + std::to_string(counter), serverPKey)){
            std::cout << "Invalid signature for server " << con_data->server_id << std::endl;
            s->close(hdl, websocketpp::close::status::policy_violation, "Server signature could not be verified.");
            if(connection_map.find(hdl) != connection_map.end()){
                connection_map.erase(hdl);
            }
            return -1;
        }else{
            std::cout << "Verified signature of server " << con_data->server_id << std::endl;
        }

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
        
        // Check if an outbound connection exists to this server
        bool outbound_connection_exists = false;
        for (const auto& pair : outbound_server_server_map) {
            if (pair.second->server_id == con_data->server_id) {
                outbound_connection_exists = true;
                break;
            }
        }

        // If no outbound connection exists, attempt to connect
        if (!outbound_connection_exists) {
            std::cout << "No outbound connection to server " << con_data->server_id 
                    << ". Attempting to establish connection." << std::endl;
            std::string server_uri = server_uris[con_data->server_id];
            int serverID = con_data->server_id;
            if (!server_uri.empty()) {
                // Start a separate thread to handle the client that connects to ws://localhost:9003
                std::thread client_thread([server_uri, serverID]() {
                    std::cout << "Starting client thread..." << "\n" << std::endl;

                    client ws_client;

                    // Set logging settings for the client
                    ws_client.set_access_channels(websocketpp::log::alevel::none);
                    ws_client.set_error_channels(websocketpp::log::elevel::none);

                    // Initialize ASIO for the client
                    ws_client.init_asio();

                    serverUtilities->connect_to_server(&ws_client, server_uri, serverID, privKey, 12345, &outbound_server_server_map);

                    ws_client.run();

                });
                // Detach the client thread so it runs independently
                client_thread.detach();
            } else {
                std::cout << "No URI found for server ID: " << con_data->server_id << std::endl;
            }
        }

        // Broadcast client updates to all servers except connecting server
        serverUtilities->broadcast_client_updates(outbound_server_server_map, global_server_list, con_data->server_id);

    }else if(data["type"] == "client_list_request"){
        // Send client list to requesting client
        serverUtilities->send_client_list(s, hdl, client_server_map, global_server_list);
    }else if(data["type"] == "client_update_request"){
        // Find requesting server's outbound connection and send client update on that
        for(const auto& connectPair: outbound_server_server_map){
            auto connection = connectPair.second;
            
            if(connection->server_id == con_data->server_id){
                serverUtilities->send_client_update(connection->client_instance, connection->connection_hdl, outbound_server_server_map, global_server_list);
            }
        }
    }else if(data["type"] == "client_update"){
        // Process client update
        global_server_list->insertServer(con_data->server_id, payload);

        // Send out client_lists to all clients
        serverUtilities->broadcast_client_lists(client_server_map, global_server_list);
    }

    std::cout << "\n";

    return 0;
}



int main(int argc, char * argv[]) {
    // Load private keys
    privKey = Server_Key_Gen::loadPrivateKey(privFileName.c_str());
    pubKey = Server_Key_Gen::loadPublicKey(pubFileName.c_str());

    // If keys files don't exist, create keys and load from newly created files
    if(privKey == nullptr || pubKey == nullptr){
        if(Server_Key_Gen::key_gen(ServerID)){
            privKey = Server_Key_Gen::loadPrivateKey(privFileName.c_str());
            pubKey = Server_Key_Gen::loadPublicKey(pubFileName.c_str());

            return 1;
        }
    }

    // Create a WebSocket++ client instance
    client ws_client;

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
                serverUtilities->connect_to_server(&ws_client, uri.second, uri.first, privKey, 12345, &outbound_server_server_map);
                
            }

            // Start the client io_service run loop
            ws_client.run();
        });

        // Detach the client thread so it runs independently
        client_thread.detach();
        
        // Listen on port 9002
        echo_server.listen(listenPort);
        
        // Start the server accept loop
        echo_server.start_accept();

        // Start the ASIO io_service run loop
        echo_server.run();

    } catch (const websocketpp::exception & e) {
        std::cout << "WebSocket++ exception: " << e.what() << std::endl;
        EVP_PKEY_free(privKey);
        EVP_PKEY_free(pubKey);
    } catch (const std::exception & e) {
        std::cout << "Standard exception: " << e.what() << std::endl;
        EVP_PKEY_free(privKey);
        EVP_PKEY_free(pubKey);
    } catch (...) {
        std::cout << "Unknown exception" << std::endl;
        EVP_PKEY_free(privKey);
        EVP_PKEY_free(pubKey);
    }
}
