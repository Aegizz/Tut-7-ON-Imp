#include "client_list.h"

ClientList::ClientList(nlohmann::json data){

    if (data.contains("servers")){
        for (const auto& server: data["servers"]){
            std::unordered_map<int, std::string> client_list;
            std::unordered_map<std::string, std::string> fingerprintsKeys;
            int server_id = server["server-id"];
            //std::cout << "Server ID: " << server_id << std::endl;
            if (server.contains("clients")){
                for (const auto& client: server["clients"]){
                    int client_id = client["client-id"];
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

// Retrieve the senders public key using their fingerprint (will be useful for signature verification on client)
std::pair<int, std::pair<int, std::string>> ClientList::retrieveClientFromFingerprint(std::string fingerprint) {
    if(clientFingerprintsKeys.find(fingerprint) != clientFingerprintsKeys.end()){
        return clientFingerprintsKeys[fingerprint];
    }else{
        return {-1, {-1, ""}};
    }
}