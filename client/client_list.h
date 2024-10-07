#ifndef client_list_h
#define client_list_h
#include <string>
#include <unordered_map>
#include <utility> //For pair
#include <nlohmann/json.hpp> // For JSON library
#include <iostream>

#include "client_key_gen.h"
#include "Fingerprint.h"

/* For implementing later on when introducing fingerprints, create a struct that points to both the public_key and SHA256(Public Key)*/

class ClientList{
    private:
    // Idea being that each server maps to another map, this ensure that we can access each server from their ID and each client from their server ID.
        std::unordered_map<int, std::unordered_map<int, std::string>> servers;
        std::unordered_map<std::string, std::pair<int, std::pair<int, std::string>>> clientFingerprintsKeys;
    public:
        ClientList(nlohmann::json data);
        std::pair<int, std::string> retrieveClient(int server_id, int client_id);
        std::pair<int, std::pair<int, std::string>> retrieveClientFromFingerprint(std::string fingerprint);
};

#endif