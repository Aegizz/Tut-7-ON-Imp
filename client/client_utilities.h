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
        static void send_hello_message(websocket_endpoint* endpoint, int id, EVP_PKEY* privKey, EVP_PKEY* pubKey, int counter);
        static void send_client_list_request(websocket_endpoint* endpoint, int id);
};

#endif