#include "server_utilities.h"

ServerUtilities::ServerUtilities(const std::string uri){
    myUri = uri;
};

// Find IP address + port number of connected client
std::string ServerUtilities::getIP(server* s, websocketpp::connection_hdl hdl){
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
bool ServerUtilities::is_connection_open(client* c, websocketpp::connection_hdl hdl){
    client::connection_ptr con = c->get_con_from_hdl(hdl);

    if(con->get_state() == websocketpp::session::state::open){
        return true;
    }
    return false;
}

// Send server hello message
int ServerUtilities::send_server_hello(client* c, websocketpp::connection_hdl hdl, EVP_PKEY* private_key, int counter){
    nlohmann::json signedMessage;

    nlohmann::json data;

    // Format server_hello message
    data["type"] = "server_hello";
    data["sender"] = myUri;
    //data["server_id"] = ServerID;

    // Should this be passed in as a string or a JSON object?
    std::string data_string = data.dump();

    signedMessage["type"] = "signed_data";
    signedMessage["data"] = data_string;
    signedMessage["counter"] = counter;
    signedMessage["signature"] = ServerSignature::generateSignature(data_string, private_key, std::to_string(counter));

    std::string message_string = signedMessage.dump();

    // Send the message via the connection
    if(!is_connection_open(c, hdl)){
        std::cout << "Connection is not open to send server hello" << std::endl;
        return 1;
    }
    websocketpp::lib::error_code ec;
    c->send(hdl, message_string, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cout << "> Error sending server hello message: " << ec.message() << std::endl;
        return 1;
    } else {
        std::cout << "> Server hello sent" << "\n" << std::endl;
        return 0;
    }
}

// Send client update request to specified connection
int ServerUtilities::send_client_update_request(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map){
    nlohmann::json request;
    request["type"] = "client_update_request";

    // Serialize JSON object
    std::string json_string = request.dump();

    if(!is_connection_open(c, hdl)){
        std::cout << "Connection is not open to send client update request" << std::endl;
        return -1;
    }

    try {
        c->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client update request to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send update request to server " << outbound_server_server_map[hdl]->server_id << " because: " << e.what() << std::endl;
        return -1;
    }

}

// Send client update to specified connection
int ServerUtilities::send_client_update(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, ServerList* global_server_list){
    std::string json_string = global_server_list->exportUpdate();

    if(!is_connection_open(c, hdl)){
        std::cout << "Connection is not open to send client update to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return -1;
    }

    try {
        c->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client update to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send client update to " << outbound_server_server_map[hdl]->server_id << " because: " << e.what() << std::endl;
        return -1;
    }
}

// Send client updates to all servers but the one specified (if specified)
void ServerUtilities::broadcast_client_updates(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, ServerList* global_server_list, int server_id_nosend){
    // Broadcast client_updates
    for(const auto& connectPair: outbound_server_server_map){
        auto connection = connectPair.second;
        if(connection->server_id != server_id_nosend){
            send_client_update(connection->client_instance, connection->connection_hdl, outbound_server_server_map, global_server_list);
        }
    }
}

// Send client list to specified connection
int ServerUtilities::send_client_list(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, ServerList* global_server_list){
    std::string json_string = global_server_list->exportClientList();

    try {
        s->send(hdl, json_string, websocketpp::frame::opcode::text);
        std::cout << "Sent client list to client " << client_server_map[hdl]->client_id <<  std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send client list because: " << e.what() << std::endl;
        return -1;
    }
}

// Send client lists to all clients but one specified (if specified)
void ServerUtilities::broadcast_client_lists(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, ServerList* global_server_list, int client_id_nosend){
    for(const auto& connectPair: client_server_map){
        auto connection = connectPair.second;
        if(connection->client_id != client_id_nosend){
            send_client_list(connection->server_instance, connection->connection_hdl, client_server_map, global_server_list);
        }
    }
}

// Send public chat to connection
int ServerUtilities::send_public_chat_server(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message){

    if(!is_connection_open(c, hdl)){
        std::cout << "Connection is not open to send public chat to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return -1;
    }

    try {
        c->send(hdl, message, websocketpp::frame::opcode::text);
        std::cout << "Sent public chat to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send public chat to server " << outbound_server_server_map[hdl]->server_id << " because: " << e.what() << std::endl;
        return -1;
    }
}

// Send public chat to all servers but specified server
void ServerUtilities::broadcast_public_chat_servers(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message, int server_id_nosend){
    for(const auto& connectPair: outbound_server_server_map){
        auto connection = connectPair.second;
        if(connection->server_id != server_id_nosend){
            send_public_chat_server(connection->client_instance, connection->connection_hdl, outbound_server_server_map, message);
        }
    }
}

// Send public chat to connection
int ServerUtilities::send_public_chat_client(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message){
    // Check if connection is open before sending

    try {
        s->send(hdl, message, websocketpp::frame::opcode::text);
        std::cout << "Sent public chat to client " << client_server_map[hdl]->client_id << std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send public chat to client " << client_server_map[hdl]->client_id << " because: " << e.what() << std::endl;
        return -1;
    }
}

// Send public chat to all clients but specified client (if specified)
void ServerUtilities::broadcast_public_chat_clients(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message, int client_id_nosend){
    for(const auto& connectPair: client_server_map){
        auto connection = connectPair.second;
        if(connection->client_id != client_id_nosend){
            send_public_chat_client(connection->server_instance, connection->connection_hdl, client_server_map, message);
        }
    }
}

// Send private chat to connection
int ServerUtilities::send_private_chat_server(client* c, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message){

    if(!is_connection_open(c, hdl)){
        std::cout << "Connection is not open to send private chat to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return -1;
    }

    try {
        c->send(hdl, message, websocketpp::frame::opcode::text);
        std::cout << "Sent private chat to server " << outbound_server_server_map[hdl]->server_id << std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send private chat to server " << outbound_server_server_map[hdl]->server_id << " because: " << e.what() << std::endl;
        return -1;
    }
}

// Send private chat to all required servers
void ServerUtilities::broadcast_private_chat_servers(std::unordered_set<std::string> serverSet, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> outbound_server_server_map, std::string message){
    for(const auto& address : serverSet){
        for(const auto& connectPair : outbound_server_server_map){
            auto connection = connectPair.second;
            std::string serverAddress = connection->server_address.substr(5, (connection->server_address.length()-5));
            
            if(serverAddress == address){
                send_private_chat_server(connection->client_instance, connection->connection_hdl, outbound_server_server_map, message);
            }
        }
    }
}

// Send private chat to client
int ServerUtilities::send_private_chat_client(server* s, websocketpp::connection_hdl hdl, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message){
    // Check if connection is open before sending

    try {
        s->send(hdl, message, websocketpp::frame::opcode::text);
        std::cout << "Sent private chat to client " << client_server_map[hdl]->client_id << std::endl;
        return 0;
    } catch (const websocketpp::exception & e) {
        std::cout << "Failed to send private chat to client " << client_server_map[hdl]->client_id << " because: " << e.what() << std::endl;
        return -1;
    }
}

// Send private chat to all clients but specified client (if specified)
void ServerUtilities::broadcast_private_chat_clients(std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal> client_server_map, std::string message, int client_id_nosend){
    for(const auto& connectPair: client_server_map){
        auto connection = connectPair.second;
        if(connection->client_id != client_id_nosend){
            send_private_chat_client(connection->server_instance, connection->connection_hdl, client_server_map, message);
        }
    }
}

// Define a function that will handle the client connections retry logic
void ServerUtilities::connect_to_server(client* c, std::string const & uri, int server_id, EVP_PKEY* private_key, int counter, std::unordered_map<websocketpp::connection_hdl, std::shared_ptr<connection_data>, connection_hdl_hash, connection_hdl_equal>* outbound_server_server_map, int retry_attempts) {
    for(const auto& connection: *outbound_server_server_map){
        if(connection.second->server_id == server_id){
            std::cout << "Outbound connection already exists to server " << server_id << std::endl;
            return;
        }
    }
    
    websocketpp::lib::error_code ec;
    client::connection_ptr con = c->get_connection(uri, ec);

    if (ec) {
        std::cout << "Could not create connection because: " << ec.message() << std::endl;
        return;
    }

    // Set fail handler to retry if connection fails
    con->set_fail_handler([this, c, uri, server_id, private_key, counter, outbound_server_server_map, retry_attempts](websocketpp::connection_hdl hdl) {
        std::cout << "Connection to " << uri << " failed, retrying in 500ms..." << std::endl;

        // Retry after 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (retry_attempts < 10) { 
            connect_to_server(c, uri, server_id, private_key, counter, outbound_server_server_map, retry_attempts + 1);
        } else {
            std::cout << "Exceeded retry limit. Giving up on connecting to " << uri << std::endl;
        }
    });

    // Set open handler when connection succeeds
    con->set_open_handler([this, c, uri, server_id, private_key, counter, outbound_server_server_map](websocketpp::connection_hdl hdl) {
        std::cout << "\nSuccessfully connected to " << uri << std::endl;

        send_server_hello(c, hdl, private_key, counter);
        
        auto con_data = std::make_shared<connection_data>();
        con_data->client_instance = c;
        con_data->connection_hdl = hdl;
        con_data->server_address = uri;
        con_data->server_id = server_id;

        // Add connection data to outbound connection map
        (*outbound_server_server_map)[hdl] = con_data;

        // Has to be here, cannot be earlier otherwise a segmentation fault occurs
        send_client_update_request(c, hdl, *outbound_server_server_map);
    });

    // Handler for when another server closes connection
    con->set_close_handler([this, c, uri, server_id, private_key, counter, outbound_server_server_map](websocketpp::connection_hdl hdl) {
        std::cout << "\nServer " << server_id << " closing outbound connection" << std::endl;

        // Erase connection from outbound connection map
        if(outbound_server_server_map->find(hdl) != outbound_server_server_map->end()){
            outbound_server_server_map->erase(hdl);
        }

        // Get connection pointer from the connection handle
        client::connection_ptr con = c->get_con_from_hdl(hdl);

        // Extract the close reason and close code
        if(con->get_remote_close_reason() != "Server signature could not be verified."){
            // Attempt to reconnect
            std::cout << "Trying to reconnect to server " << server_id << std::endl;
            connect_to_server(c, uri, server_id, private_key, counter, outbound_server_server_map);
        }else{
            std::cout << "Invalid signature sent in hello" << std::endl;
            return;
        }
    });

    // Try to connect to the server
    c->connect(con);
}
