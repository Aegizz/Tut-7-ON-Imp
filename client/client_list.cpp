#include "client_list.h"

ClientList::ClientList(nlohmann::json data){

    if (data.contains("servers")){
        for (const auto& server: data["servers"]){
            std::unordered_map<int, std::string> client_list;
            int server_id = server["server-id"];
            std::cout << "Server ID: " << server_id << std::endl;
            if (server.contains("clients")){
                for (const auto& client: server["clients"]){
                    int client_id = client["client-id"];
                    std::string public_key = client["public-key"];

                    std::cout << "  Client ID: " << client_id << std::endl;
                    std::cout << "  Public Key: " << public_key << std::endl;

                    client_list.insert(std::pair<int, std::string>(client_id, public_key));
                }
                servers.insert(std::pair<int, std::unordered_map<int, std::string>>(server_id, client_list));

            }
            
        }
    }
}

int ClientList::addClient(){

}

int ClientList::removeClient(){

}

std::pair<int,std::string> ClientList::retrieveClient(){

}

int ClientList::updateClientList(nlohmann::json data){

}