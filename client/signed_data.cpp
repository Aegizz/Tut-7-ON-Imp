#include "signed_data.h"

void SignedData::sendSignedMessage(std::string data, EVP_PKEY * private_key, websocket_endpoint* endpoint, int id, int counter){
    nlohmann::json message;

    message["type"] = "signed_data";
    message["data"] = data;
    message["counter"] = counter;
    message["signature"] = ClientSignature::generateSignature(data, private_key, std::to_string(counter));

    std::string json_string = message.dump();


    /* When  */
    connection_metadata::ptr metadata = endpoint->get_metadata(id);

    if (!(metadata->get_status() == "Open")){
        return;
    }
    websocketpp::lib::error_code ec;
    endpoint->send(id, json_string, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cerr << "> Error sending signed_data message: " << ec.message() << std::endl;
    } else {
        std::cout << "> signed_data message sent" << std::endl;
    }
}

/*Function to decrypt a signed messsage's content, does not verify the reciever or check counter
  Ensure the entire message is provided.
*/
std::string SignedData::decryptSignedMessage(std::string message, EVP_PKEY * private_key) {
    // Parse the JSON message
    nlohmann::json message_json = nlohmann::json::parse(message);
    if (message_json["type"] != "signed_data") {
        std::cerr << "Not signed data!" << std::endl;
        return "";
    }

    // Extract the data field from the JSON message
    nlohmann::json data = message_json["data"];

    std::vector<std::string> aesKeys;
    std::string decrypted_key;

    // Iterate over the encrypted symmetric keys
    for (const auto& element : data["symm_keys"]) {
        std::string key_dump = element.dump();
        const unsigned char * encrypted_key = reinterpret_cast<const unsigned char*>(key_dump.c_str());
        unsigned char * decrypted = nullptr;
        int decrypted_length = Client_Key_Gen::rsaDecrypt(private_key, encrypted_key, key_dump.size(), &decrypted);
        
        // If decryption is successful
        if (decrypted_length > 0 && decrypted != nullptr) {
            decrypted_key = std::string(reinterpret_cast<char*>(decrypted), decrypted_length);
        

            // Create vectors to hold the key, ciphertext, iv, and tag
            std::vector<unsigned char> key(decrypted_key.begin(), decrypted_key.end()); // Convert decrypted key to vector
            std::vector<unsigned char> ciphertext(data["chat"].begin(), data["chat"].end());
            std::vector<unsigned char> iv(data["iv"].begin(), data["iv"].end());
            // Check if ciphertext is large enough to contain the tag
            if (ciphertext.size() < 16) {
                std::cerr << "Ciphertext too short to contain authentication tag!" << std::endl;
                return "";
            }

            // Split the ciphertext: last 16 bytes are the tag, the rest is the actual ciphertext
            std::vector<unsigned char> tag(ciphertext.end() - 16, ciphertext.end());
            std::vector<unsigned char> actual_ciphertext(ciphertext.begin(), ciphertext.end() - 16);

            std::vector<unsigned char> decrypted_text;
            // Perform AES GCM decryption
            if (!aes_gcm_decrypt(actual_ciphertext, key, iv, tag, decrypted_text)) {
                std::cerr << "Decryption failed!" << std::endl;
                return "";
            }

            // Convert the decrypted text back to string
            std::string decrypted_message(decrypted_text.begin(), decrypted_text.end());
            return decrypted_message;
        }
    }
    std::cerr << "Could not decrypt the message" << std::endl;
    return "";
}

