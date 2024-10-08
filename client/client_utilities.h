#ifndef CLIENT_UTILITIES_H
#define CLIENT_UTILITIES_H
#include <string>
// For UTC timestamp
#include <chrono>
#include <ctime> 
#include <sstream>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "websocket_endpoint.h"
#include "websocket_metadata.h"

#include <websocketpp/common/thread.hpp>

#include "MessageGenerator.h" // For creating messages to send
#include "client_key_gen.h" // OpenSSL Key generation


class ClientUtilities{
    public:
        static std::string get_ttd();
        static bool is_connection_open(websocket_endpoint* endpoint, int id);

        /*
            Calls MessageGenerator::helloMessage() and sends it to server.
            Refer to ClientDocumentation.md for more details.

            websocket_endpoint* endpoint - Connection to server 
            int id - Local ID for connection
        */
        static void send_hello_message(websocket_endpoint* endpoint, int id, EVP_PKEY* privKey, EVP_PKEY* pubKey, int counter);

        /*
            Calls MessageGenerator::clientListRequestMessage() and sends it to server.
            Refer to ClientDocumentation.md for more details.

            websocket_endpoint* endpoint - Connection to server 
            int id - Local ID for connection
        */
        static void send_client_list_request(websocket_endpoint* endpoint, int id);

        /*
            Calls MessageGenerator::publicChatMessage() and sends it to server.
            Refer to ClientDocumentation.md for more details.

            websocket_endpoint* endpoint - Connection to server 
            int id - Local ID for connection
            std::string message - Message to send
            EVP_PKEY* privKey - Your private key
            EVP_PKEY* pubKey - Your public key
            int counter - Current counter value
        */
        static void send_public_chat(websocket_endpoint* endpoint, int id, std::string message, EVP_PKEY* privKey, EVP_PKEY* pubKey, int counter);

        /*
            Calls MessageGenerator::chatMessage() and sends it to server.
            Refer to ClientDocumentation.md for more details.

            websocket_endpoint* endpoint - Connection to server 
            int id - Local ID for connection
            std::string message - Message to send
            EVP_PKEY* privKey - Your private key
            EVP_PKEY* pubKey - Your public key
            std::vector<EVP_PKEY*> their_public_keys - List of their public keys
            std::vector<std::string> destination_servers_vector
            int counter - Current counter value
        */
        static void send_chat(websocket_endpoint* endpoint, int connection_id, std::string message, EVP_PKEY* privKey, EVP_PKEY* pubKey, std::vector<EVP_PKEY*> their_public_keys, std::vector<std::string> destination_servers_vector, int counter, int client_id=0, int server_id=0);
};

#endif