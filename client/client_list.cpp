#include "client_list.h"

ClientList::ClientList(){}

ClientList::ClientList(nlohmann::json data){

    if (data.contains("servers")){
        for (const auto& server: data["servers"]){
            std::unordered_map<int, std::string> client_list;
            int server_id = server["server-id"];
            //std::cout << "Server ID: " << server_id << std::endl;
            if (server.contains("clients")){
                for (const auto& client: server["clients"]){
                    int client_id = client["client-id"];
                    std::string public_key = client["public-key"];

                    //std::cout << "  Client ID: " << client_id << std::endl;
                    //std::cout << "  Public Key: " << public_key << std::endl;

                    client_list.insert(std::pair<int, std::string>(client_id, public_key));
                }
                servers.insert(std::pair<int, std::unordered_map<int, std::string>>(server_id, client_list));

            }
            
        }
    }
}

std::pair<int, std::string> ClientList::retrieveClient(int server_id, int client_id) {
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

void ClientList::insertClient(int server_id, std::string public_key){
    clientID++;

    // Add client to map
    servers[server_id][clientID] = public_key;
}

std::string ClientList::exportJSON(){
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
        serverJSON["address"] = "<Address of server>";

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