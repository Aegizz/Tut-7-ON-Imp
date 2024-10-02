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
        std::unordered_map<int, std::unordered_map<int, std::string>> servers; // Servers stored against their ID, map of clients stored against their IDs
        std::unordered_map<int, std::string> currentClients; // Clients currently connected to THIS server
        std::unordered_map<int, std::string> knownClients; // Clients that belong to this server

        // Temporary way to store server addresses against their ID
        std::unordered_map<int, std::string> serverAddresses = {{1, "127.0.0.1:9002"}, {2, "127.0.0.1:9003"}};

        void save_mapping_to_file();
        void load_mapping_from_file();

        int my_server_id;
        int clientID=1000;
    public:
        ServerList(int server_id);

        int ObtainID(std::string address);

        std::pair<int, std::string> retrieveClient(int server_id, int client_id);

        int insertClient(std::string public_key);
        void removeClient(int client_id);
        void insertServer(int server_id, std::string update);
        void removeServer(int server_id);

        std::string exportUpdate();
        std::string exportClientList();
};

#endif