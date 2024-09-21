#include <iostream>
#include <nlohmann/json.hpp>
#include "../client/client_list.h"

int main() {
    // Test JSON string
    std::string json_str = R"({
        "type": "client_list",
        "servers": [
            {
                "address": "192.168.1.1",
                "server-id": 1,
                "clients": [
                    {
                        "client-id": 1001,
                        "public-key": "RSA_PUBLIC_KEY_1"
                    },
                    {
                        "client-id": 1002,
                        "public-key": "RSA_PUBLIC_KEY_2"
                    }
                ]
            },
            {
                "address": "192.168.1.2",
                "server-id": 2,
                "clients": [
                    {
                        "client-id": 2001,
                        "public-key": "RSA_PUBLIC_KEY_3"
                    }
                ]
            }
        ]
    })";

    // Parse the JSON
    nlohmann::json data = nlohmann::json::parse(json_str);

    // Create a ClientList object and pass the JSON data
    ClientList client_list(data);

    // Test retrieveClient method
    try {
        auto client = client_list.retrieveClient(1, 1001);
        std::cout << "Retrieved Client ID: " << client.first << ", Public Key: " << client.second << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        auto client = client_list.retrieveClient(2, 2001);
        std::cout << "Retrieved Client ID: " << client.first << ", Public Key: " << client.second << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    // Test error case
    try {
        auto client = client_list.retrieveClient(3, 3001);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
