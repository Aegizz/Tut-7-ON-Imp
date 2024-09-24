#ifndef server_list_h
#define server_list_h
#include <string>
#include <unordered_map>
#include <utility> //For pair
#include <nlohmann/json.hpp> // For JSON library
#include <iostream>
#include <fstream>
class ServerList{
    private:
    // Idea being that each server maps to another map, this ensure that we can access each server from their ID and each client from their server ID.
        std::unordered_map<int, std::unordered_map<int, std::string>> servers;
        std::unordered_map<int, std::string> currentClients;
        std::unordered_map<int, std::string> knownClients;

        void save_mapping_to_file();
        void load_mapping_from_file();

        int my_server_id;
        int clientID=1000;
    public:
        ServerList(int server_id);

        std::pair<int, std::string> retrieveClient(int server_id, int client_id);

        int insertClient(std::string public_key);
        void removeClient(int client_id);

        std::string exportUpdate();
        std::string exportClientList();
};

#endif