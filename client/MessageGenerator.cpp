#include "MessageGenerator.h"

#include "ChatMessage.h"
#include "DataMessage.h"
#include "Fingerprint.h"
#include "HelloMessage.h"
#include "PublicChatMessage.h"
#include "client_signature.h"


std::string MessageGenerator::chatMessage(std::string message, EVP_PKEY * your_private_key, EVP_PKEY * your_public_key, std::vector<EVP_PKEY*> their_public_keys, std::vector<std::string> destination_servers_vector, int client_id, int server_id, int counter){
    
    std::string data; // To hold the encrypted chat message
    std::vector<std::string> fingerprints; // Store fingerprints of participants

    // Add your own fingerprint first
    fingerprints.push_back(Fingerprint::generateFingerprint(your_public_key));

    // Add the fingerprints of the recipients
    for (auto key: their_public_keys){
        fingerprints.push_back(Fingerprint::generateFingerprint(key));
    }

    // Generate a chat message in JSON format, embedding the message and participant fingerprints
    std::string chat_message = ChatMessage::generateChatMessage(message, fingerprints);

    // Encrypt the chat message using the recipient's public keys and destination server list
    data = DataMessage::generateDataMessage(chat_message, their_public_keys, destination_servers_vector);

    // Sign the encrypted message with the client's private key and a message counter
    nlohmann::json signed_message;

    signed_message["type"] = "signed_data";
    signed_message["data"] = data;
    signed_message["counter"] = counter;
    signed_message["signature"] = ClientSignature::generateSignature(data, your_private_key, std::to_string(counter));

    return signed_message.dump(); // Return the signed and encrypted message
}


std::string MessageGenerator::clientListRequestMessage(){
    
    nlohmann::json client_list_request; // Define a JSON object to hold the request

    // Set the type of message to 'client_list_request'
    client_list_request["type"] = "client_list_request";

    // Convert the JSON object to a string and return it
    return client_list_request.dump();
}


std::string MessageGenerator::helloMessage(EVP_PKEY * your_private_key ,EVP_PKEY * your_public_key, int counter){
    
    std::string hello_message;

    // Generate the Hello message which includes the client's public key
    hello_message = HelloMessage::generateHelloMessage(your_public_key);

    nlohmann::json signed_message;

    signed_message["type"] = "signed_data";
    signed_message["data"] = hello_message;
    signed_message["counter"] = counter;
    signed_message["signature"] = ClientSignature::generateSignature(hello_message, your_private_key, std::to_string(counter));

    return signed_message.dump(); // Return the signed Hello message
}


std::string MessageGenerator::publicChatMessage(std::string message, EVP_PKEY * your_private_key, EVP_PKEY * your_public_key, int counter){
    
    // Create a public chat message that includes the message and the client's public key
    std::string data = PublicChatMessage::generatePublicChatMessage(message, your_public_key);
    
    nlohmann::json signed_message;

    signed_message["type"] = "signed_data";
    signed_message["data"] = data;
    signed_message["counter"] = counter;
    signed_message["signature"] = ClientSignature::generateSignature(data, your_private_key, std::to_string(counter));

    return signed_message.dump(); // Return the signed Hello message
}
