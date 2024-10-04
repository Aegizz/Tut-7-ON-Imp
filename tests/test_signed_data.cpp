#include <iostream>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "../client/signed_data.h"  // Ensure this path is correct
#include "../client/client_list.h"
#include "../client/client.h"
#include "../client/client_key_gen.h"
#include "../client/websocket_endpoint.h"
#include "../client/websocket_metadata.h"
#include "../client/DataMessage.h"

using websocketpp::connection_hdl;

typedef websocketpp::server<websocketpp::config::asio> server;

void testSendAndReceive(SignedData& signedData, EVP_PKEY* privateKey, std::vector<EVP_PKEY *> recipent_pub_keys, std::vector<std::string> server_addresses, websocket_endpoint* endpoint, int id) {
    // Sample data to send
    std::string data = DataMessage::generateDataMessage("Hello world!", recipent_pub_keys, server_addresses);
    int counter = 1;

    // Sending a signed message
    signedData.sendSignedMessage(data, privateKey, endpoint, id, counter);


    nlohmann::json message_json;

    message_json["type"] = "signed_data";
    message_json["data"] = data;
    message_json["counter"] = counter;
    message_json["signature"] = ClientSignature::generateSignature(data, privateKey, std::to_string(counter));

    std::string json_string = message_json.dump();

    // Create a mock signed message for testing decryption

    std::string mockMessageStr = message_json.dump();

    // Attempt to decrypt the signed message
    std::string decryptedMessage = signedData.decryptSignedMessage(mockMessageStr, privateKey);

    // Output the result
    if (!decryptedMessage.empty()) {
        std::cout << "Decrypted message: " << decryptedMessage << std::endl;
    } else {
        std::cout << "Decryption failed or returned empty." << std::endl;
    }
}

ClientList * global_client_list = nullptr;

int main() {
    // Initialize WebSocket server (or endpoint) here
    websocket_endpoint endpoint;
    // You would typically set up the endpoint, including any necessary configurations.
    
    // Mock connection metadata
    int id = endpoint.connect("ws://localhost:9002", global_client_list); // Example connection ID

    // Load or create a private key for testing
    EVP_PKEY* privateKey = Client_Key_Gen::loadPrivateKey("private_key.pem");
    EVP_PKEY * publicKey = Client_Key_Gen::loadPublicKey("public_key.pem");
    if (!privateKey) {
        std::cerr << "Failed to load private key!" << std::endl;
        return 1; // Exit if key loading fails
    }

    // Create an instance of SignedData
    SignedData signedData;

    // Test sending and receiving messages
    testSendAndReceive(signedData, privateKey, std::vector<EVP_PKEY*>({publicKey}),std::vector<std::string>({"ws://localhost::9002"}),&endpoint, id);

    // Clean up resources (if needed)
    EVP_PKEY_free(privateKey); // Free the private key when done

    return 0;
}
