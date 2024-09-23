#ifndef client_list_h
#define client_list_h
#include <string>
#include <unordered_map>
#include <utility> //For pair
#include <nlohmann/json.hpp> // For JSON library
#include <iostream>
class ClientList{
    private:
    // Idea being that each server maps to another map, this ensure that we can access each server from their ID and each client from their server ID.
        std::unordered_map<int, std::unordered_map<int, std::string>> servers;
        int clientID=1000;
    public:
        ClientList();
        ClientList(nlohmann::json data);
        std::pair<int, std::string> retrieveClient(int server_id, int client_id);
        std::string exportJSON();
        void insertClient(int server_id, std::string public_key);

};

#endif