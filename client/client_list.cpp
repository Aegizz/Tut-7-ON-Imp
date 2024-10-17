#include "client_list.h"

ClientList::ClientList(){}

// Clears the client list and other maps and updates the client list using the new client list message received.
// Calculates fingerprint of each client and stores it against a pair of pairs <server_id <client_id, public_key>>
// Stores an unordered_map<client_id, public_key> against each server's ID
// Stores a map of server addresses against each server's ID 
void ClientList::update(nlohmann::json data){
    servers.clear();
    clientFingerprintsKeys.clear();
    serverAddresses.clear();

    if (data.contains("servers")){
        for (const auto& server: data["servers"]){
            std::unordered_map<int, std::string> client_list;
            std::unordered_map<std::string, std::string> fingerprintsKeys;
            int server_id = server["server-id"];
            serverAddresses[server_id] = server["address"];
            //std::cout << "Server ID: " << server_id << std::endl;
            if (server.contains("clients")){
                for (const auto& client: server["clients"]){
                    if (client["client-id"]){
                        int client_id = client["client-id"];
                        if (client_id > clientCount){
                            clientCount = client_id + 1;
                        }
                    } else {
                        clientCount++;
                        int client_id = clientCount;
                    }
                    std::string public_key = client["public-key"];

                    std::string fingerprint = Fingerprint::generateFingerprint(Client_Key_Gen::stringToPEM(public_key));
                    std::pair<int, std::string> clientIDKey(client_id, public_key);
                    clientFingerprintsKeys[fingerprint] = std::pair<int, std::pair<int, std::string>>(server_id, clientIDKey);

                    //std::cout << "  Client ID: " << client_id << std::endl;
                    //std::cout << "  Public Key: " << public_key << std::endl;

                    client_list.insert(std::pair<int, std::string>(client_id, public_key));
                }
                servers.insert(std::pair<int, std::unordered_map<int, std::string>>(server_id, client_list));
            }
            
        }
    }
}

// Retrieves a server address using a server ID. Returns an empty string if the server ID is invalid.
std::string ClientList::retrieveAddress(int server_id){
    if(serverAddresses.find(server_id) != serverAddresses.end()){
        return serverAddresses[server_id];
    }
    return "";
}

// Prints all users connected in neighbourhood to terminal, labelling the user with the matching server_id and client_id parameters as "You"
void ClientList::printUsers(int server_id, int client_id){
    std::cout << "Connected Users" << std::endl;
    for(const auto& server: servers){
        int serverID = server.first;
        for(const auto& client: server.second){
            int clientID = client.first;
            if(serverID == server_id && clientID == client_id){
                std::cout << "You are ServerID: " << serverID << " ClientID: " << clientID << std::endl;   
            }else{
                std::cout << "ServerID: " << serverID << " ClientID: " << clientID << std::endl;
            }  
        }
    }
}

// Retrieves a pair of a client's client id and public key using
std::pair<int, std::string> ClientList::retrieveClient(int server_id, int client_id) {
    // Check if the server exists
    if (servers.find(server_id) != servers.end()) {
        // Check if the client exists in the server
        auto& client_list = servers[server_id];
        if (client_list.find(client_id) != client_list.end()) {
            return {client_id, client_list[client_id]};
        } else {
            std::cerr << "Client ID not found." << std::endl;
            return {server_id, ""};
        }
    } else {
        std::cerr << "Server ID not found." << std::endl;
        return {-1, ""};
    }
}

// Retrieve the senders public key using their fingerprint (will be useful for signature verification on client)
std::pair<int, std::pair<int, std::string>> ClientList::retrieveClientFromFingerprint(std::string fingerprint) {
    if(clientFingerprintsKeys.find(fingerprint) != clientFingerprintsKeys.end()){
        return clientFingerprintsKeys[fingerprint];
    }else{
        return {-1, {-1, ""}};
    }
}