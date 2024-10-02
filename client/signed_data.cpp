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
    std::cout << "Parsing json message..." << std::endl;
    nlohmann::json message_json = nlohmann::json::parse(message);
    if (message_json["type"] != "signed_data") {
        std::cerr << "Not signed data!" << std::endl;
        return "";
    }
    nlohmann::json data;
    // Extract the data field from the JSON message
    if (message_json.contains("data")) {
        std::cout << "Data: " << message_json["data"] << std::endl;

        // Check if 'data' is a string
        if (message_json["data"].is_string()) {
            // Parse the string as JSON
            try {
                data = nlohmann::json::parse(message_json["data"].get<std::string>());
                // Now you can work with 'data' as a JSON object
                std::cout << "Parsed data: " << data.dump(4) << std::endl;
                
                // Proceed with your operations on `data`
                // For example, accessing specific fields

                
                // You can continue with decryption or other operations here
                // decryptData(data);  // Assuming you have a function to decrypt the data
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "Failed to parse JSON from string: " << e.what() << std::endl;
                return "";
            }
        } else {
            std::cerr << "'data' is not a string." << std::endl;
            return "";
        }
    } else {
        std::cerr << "'data' key does not exist in message_json." << std::endl;
        return "";
    }
    std::vector<std::string> aesKeys;
    std::string decrypted_key;

    // Iterate over the encrypted symmetric keys
    std::cout << "Iterating over symm_keys to find key..." << std::endl;

    if (data["symm_keys"].is_array()) {
        std::cout << "Creating element..." << std::endl;
        sleep(5);
        for (const auto& element : data["symm_keys"]) {
            if  (!element.is_string()){
                std::cerr << "Element is not an object!" << std::endl;
            } else {
                std::cout << "Element: " << element << std::endl;
                std::string key_dump = Base64::decode(element);
                std::cout << "Key dump: " << key_dump << std::endl;
                std::cout << "Key length: " << key_dump.length() << std::endl;
                const unsigned char * encrypted_key = reinterpret_cast<const unsigned char*>(key_dump.c_str());
                unsigned char * decrypted = nullptr;
                int decrypted_length = Client_Key_Gen::rsaDecrypt(private_key, encrypted_key, key_dump.size(), &decrypted);
                std::cout << "Decrypted data: " << decrypted << std::endl;
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
                        std::cerr << "\nDecryption failed!: AES" << std::endl;
                        return "";
                    }

                    // Convert the decrypted text back to string
                    std::string decrypted_message(decrypted_text.begin(), decrypted_text.end());
                    return decrypted_message;
                } else {
                    std::cerr << "\nDecryption failed!: RSA" << std::endl;
                }
            }

        }
    }  else {
        std::cerr << "symm_keys is not an array!" <<std::endl;
    }
    std::cerr << "Could not decrypt the message" << std::endl;
    return "";
}

