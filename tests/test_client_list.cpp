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
                        "public-key": "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1Kr2yMNoI6X4wlQnz+ih\nwFM7AiSYyLV7voNOCBL9gQW4l/8bFfW+0LYQt21lkDilb86steO3T2+vEYfRmNpp\nVF5mXf04RROXCAEQ+JJnuZjKkpLd8FVMJor/wONk+M/AN/hVXshw1qES+dvB3oBB\n0RZ6jl/v5/peEywFOjTXOk2KP85jGYTOCjhz0JyZtyxbbTTPW2RCsQdCfmL4NyY3\neonq7hYhKMAf4iIh3YRYXY3wBxeeRVzJDsD5Gx0r5FFJ7XxhHKy10uydFEnBxiyN\n6p3iA3hW0va16rv1HEMMFP4cGMCL3T7wPEnvB1sP11DILh+pPEBW6OGllOM8Nt7Q\n0QIDAQAB\n-----END PUBLIC KEY-----\n"
                    },
                    {
                        "client-id": 1002,
                        "public-key": "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2bXtoCciXOc9UjkXu6Vi\n5LVD12mH5/mHkqjb83TZyZ7G8W6yCtcHYLdlKrbnda0sk2E1ZqigH6JlR7hqL9fB\ntEW3EVX8mLiGzIkEE83DJ1snF1KU+S1E3CVf3eLfveqbpSWoUZs7etytz7UpeVAM\nLXgBtRvXa/GFuXLUZ25BRgKGdjKa83/yBCUvEDblP7jQEU/1e6Uuybolv/M3Zgfx\ni3PBgNs1FL7D0BPDqSPs1e0CemK3uoGnbkutkHYla9q9hHq+elF4SVxvbkxcJRZO\nfmAtwCyJxjf7eoK/hmqtAQj0iQYxSjV4Qf7z8zVRcntuBAOyx3eEKwILKGVR/IYv\ngwIDAQAB\n-----END PUBLIC KEY-----\n"
                    }
                ]
            },
            {
                "address": "192.168.1.2",
                "server-id": 2,
                "clients": [
                    {
                        "client-id": 2001,
                        "public-key": "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzbLbxKEvJMrdXApvNdrt\npcyX/NFJJJKfnYAptpUZ5PYiU4d0TAbIPtglLz5CMdTxFEK/P++PvDyjNZ3Q6Tjw\n7HplruDJ5X7SyLHAoVTSjl0H+bFslvU1hH0EVsp49R88qW2/Uf1n96KaW9dJbR/W\nU9Tcfx4FukjDCiLLA90RrjBdf5bBe8ZeURuNJ3OISj0UTWaHQom+MWehrS5JiZtj\nUaRfMLSj5SwFhzOUdrhW/sKep1jTuPUlCwx+9WAsZs8WrRqkYib9xuSypGTH9cUT\neSpMnugdH2QWS1LFMGuNZ/zNouRzUMZHdJoV6BJvzMa9UyVc5+VmDbzKVV6rfcG8\n9QIDAQAB\n-----END PUBLIC KEY-----\n"
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
        auto client = client_list.retrieveClient(2, 2001);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
