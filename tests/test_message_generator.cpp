#include <iostream>
#include <vector>
#include <openssl/evp.h>
#include "../client/MessageGenerator.h"
#include "../client/client_key_gen.h"

int main() {
    // Step 1: Generate or load your private and public keys
    EVP_PKEY* your_private_key = Client_Key_Gen::loadPrivateKey("private_key.pem"); // Simulate loading/generating a private key
    EVP_PKEY* your_public_key = Client_Key_Gen::loadPublicKey("public_key.pem"); // Extract public key from private

    // Step 2: Generate recipient public keys
    std::vector<EVP_PKEY*> their_public_keys;
    for (int i = 0; i < 3; ++i) {
        // Simulate generating/receiving public keys of recipients
        their_public_keys.push_back(Client_Key_Gen::loadPublicKey(("public_key_" + std::to_string(i) + ".pem").c_str())); 
    }

    // Step 3: Destination servers for the message
    std::vector<std::string> destination_servers_vector = {"localhost:9002", "localhost:9003"};

    // Step 4: Create client and server IDs (simulate)
    int client_id = 1001;
    int server_id = 2001;

    // Step 5: Initialize message counter
    int counter = 1;

    // Step 6: Create a chat message
    std::string message = "Hello, this is a test chat message!";
    std::string signed_chat_message = MessageGenerator::chatMessage(
        message, 
        your_private_key, 
        your_public_key, 
        their_public_keys, 
        destination_servers_vector, 
        client_id, 
        server_id, 
        counter
    );

    std::cout << "Signed Chat Message: " << signed_chat_message << std::endl;

    // Step 7: Create a client list request message
    std::string client_list_request = MessageGenerator::clientListRequestMessage();
    std::cout << "Client List Request Message: " << client_list_request << std::endl;

    // Step 8: Create a hello message
    std::string hello_message = MessageGenerator::helloMessage(your_private_key, your_public_key, counter);
    std::cout << "Signed Hello Message: " << hello_message << std::endl;

    // Step 9: Create a public chat message
    std::string public_chat_message = "This is a public announcement!";
    std::string signed_public_chat_message = MessageGenerator::publicChatMessage(public_chat_message, your_private_key, your_public_key, counter);
    std::cout << "Signed Public Chat Message: " << signed_public_chat_message << std::endl;

    // Cleanup OpenSSL keys
    EVP_PKEY_free(your_private_key);
    EVP_PKEY_free(your_public_key);
    for (auto& key : their_public_keys) {
        EVP_PKEY_free(key);
    }

    return 0;
}
