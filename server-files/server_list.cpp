#include "server_list.h"

// Initialise server list with inputted server id and load from mapping file
ServerList::ServerList(int server_id){
    // Set my server id
    my_server_id = server_id;

    load_mapping_from_file();
}

int ServerList::ObtainID(std::string address){
    for(const auto& server: serverAddresses){
        if(server.second == address){
            return server.first;
        }
    }
    return -1;
}

// Saves the known users to a mapping file
void ServerList::save_mapping_to_file() {
    // Generate mapping file name
    std::string filename = "server-files/server_mapping";
    filename.append(std::to_string(my_server_id));
    filename.append(".json");

    // Save the map to a file
    nlohmann::json j_map = knownClients;
    std::ofstream file(filename);
    file << j_map.dump(4);
}

// Reads the known users for this server from a mapping file
void ServerList::load_mapping_from_file() {
    // Generate mapping file name
    std::string filename = "server-files/server_mapping";
    filename.append(std::to_string(my_server_id));
    filename.append(".json");

    // Load the map from file name
    std::ifstream file(filename);
    // Check if file exists
    if(!file){
        return;
    }

    // Parse file to JSON object
    nlohmann::json j_map;
    file >> j_map;

    // Convert JSON object to map and store
    knownClients = j_map.get<std::unordered_map<int, std::string>>();

    // Find and set last ID used so new clients get correct ID
    int largestID=0;
    for(const auto& client: knownClients){
        if(client.first > largestID){
            largestID = client.first;
        }
    }
    clientID = largestID;
}

// Retrieves a client's public key using its server and client ids
std::pair<int, std::string> ServerList::retrieveClient(int server_id, int client_id) {
    // Check if the server exists
    if (servers.find(server_id) != servers.end()) {
        // Check if the client exists in the server
        auto& client_list = servers[server_id];
        if (client_list.find(client_id) != client_list.end()) {
            return {client_id, client_list[client_id]};
        } else {
            throw std::runtime_error("Client ID not found.");
        }
    } else {
        throw std::runtime_error("Server ID not found.");
    }
}

// Inserts a client to the list when a new connection is established
int ServerList::insertClient(std::string public_key){
    // Check list of known clients to see if client's public key matches one stored 
    for(const auto& client: knownClients){
        // If the client's public key matches, use previous ID
        if(client.second  == public_key){
            servers[my_server_id][client.first] = public_key;
            currentClients[client.first] = public_key;

            return client.first;
        }
    }
    // Otherwise create new client
    clientID++;

    // Add client to maps
    servers[my_server_id][clientID] = public_key;
    currentClients[clientID] = public_key;

    // Add new client to map of known clients and save new map to file
    knownClients[clientID] = public_key;
    save_mapping_to_file();
    
    return clientID;
}

// Removes a client from the list when the connection is dropped
void ServerList::removeClient(int client_id){
    // Remove client from maps
    servers[my_server_id].erase(client_id);

    currentClients.erase(client_id);
}

// Removes a server from the list
void ServerList::removeServer(int server_id){
    servers.erase(server_id);
}

// Inserts or replaces a server in the list using a client update
void ServerList::insertServer(int server_id, std::string update){

    // Convert string to JSON object
    nlohmann::json updatedServerJSON = nlohmann::json::parse(update);

    nlohmann::json clientsArray = updatedServerJSON["clients"];

    std::unordered_map<int, std::string> updatedServer;

    for(const auto& client: clientsArray){
        updatedServer[client["client_id"]] = client["public_key"];
    }

    // Store map
    servers[server_id] = updatedServer;
}

// Creates a JSON client list of current connected network
// Meant to be used for client_list
std::string ServerList::exportClientList(){
    // Create client list JSON object
    nlohmann::json clientList;

    // Set type
    clientList["type"] = "client_list";

    // Create array of JSON objects for all connected servers
    nlohmann::json serversJSON = nlohmann::json::array();

    for (const auto& server: servers){
        // Create JSON object for connected server
        nlohmann::json serverJSON;

        // Set fields
        // Temporary way to find server addresses using ID
        serverJSON["address"] = serverAddresses[server.first];

        // Modified this to be a given number as it needs to be a number for the client to store.
        serverJSON["server-id"] = server.first;

        // Create array of JSON objects for clients connected to server
        nlohmann::json serverClients = nlohmann::json::array();

        for(const auto& client: server.second){
            // Build client JSON object
            nlohmann::json clientsJSON;
            
            // Modified this to be a given number as it needs to be a number for the client to store.
            clientsJSON["client-id"] = client.first;
            clientsJSON["public-key"] = client.second;
            
            // Push to array of clients of server
            serverClients.push_back(clientsJSON);
        }

        // Set clients field to array of clients in server
        serverJSON["clients"] = serverClients;

        // Push back array server to array of servers
        serversJSON.push_back(serverJSON);
    }
     // Add servers JSON array
    clientList["servers"] = serversJSON;

    // Serialize JSON object
    std::string json_string = clientList.dump();

    return json_string;
}

// Creates a JSON list of clients currently connected to servers
// Meant to be used for client_update
std::string ServerList::exportUpdate(){
    // Create client list JSON object
    nlohmann::json clientUpdate;

    // Set type
    clientUpdate["type"] = "client_update";

    // Create array of JSON objects for all clients
    nlohmann::json clientsArray = nlohmann::json::array();

    // Build array of client public keys
    for(const auto& client: currentClients){
        nlohmann::json clientJSON;
        clientJSON["client_id"] = client.first;
        clientJSON["public_key"] = client.second;

        clientsArray.push_back(clientJSON);
    }

    clientUpdate["clients"] = clientsArray;

    // Serialize JSON object
    std::string json_string = clientUpdate.dump();

    return json_string;
}