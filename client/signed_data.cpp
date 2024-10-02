#include "signed_data.h"

std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    size_t length = hex.length();

    // Ensure the length of hex string is even
    if (length % 2 != 0) {
        throw std::invalid_argument("Hex string must have an even length.");
    }

    for (size_t i = 0; i < length; i += 2) {
        std::string byteString = hex.substr(i, 2); // Get two characters
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}


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
    nlohmann::json data;
    // Extract the data field from the JSON message
    if (message_json.contains("data")) {
        // Check if 'data' is a string
        if (message_json["data"].is_string()) {
            // Parse the string as JSON
            try {
                data = nlohmann::json::parse(message_json["data"].get<std::string>());
                // Now you can work with 'data' as a JSON object                
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

    if (data["symm_keys"].is_array()) {

        for (const auto& element : data["symm_keys"]) {
            if  (!element.is_string()){
                std::cerr << "Element is not an object!" << std::endl;
            } else {
                std::string key_dump = Base64::decode(element);

                const unsigned char * encrypted_key = reinterpret_cast<const unsigned char*>(key_dump.c_str());
                unsigned char * decrypted = nullptr;
                int decrypted_length = Client_Key_Gen::rsaDecrypt(private_key, encrypted_key, key_dump.size(), &decrypted);
                // If decryption is successful
                if (decrypted_length > 0 && decrypted != nullptr) {
                    decrypted_key = std::string(reinterpret_cast<char*>(decrypted), decrypted_length);
                

                    // Create vectors to hold the key, ciphertext, iv, and tag
                    std::cout << "Key: " << decrypted_key << std::endl;
                    std::vector<unsigned char> key = hexToBytes(decrypted_key); // Convert decrypted key to vector
                    std::string iv_str = Base64::decode(data["iv"].get<std::string>());
                    std::cout << "iv: "<< iv_str << std::endl;

                    std::vector<unsigned char> iv = hexToBytes(iv_str);


                    if (data.contains("chat") && !data["chat"].is_null() && data["chat"].is_string()) {
                        
                        // Decode the base64 encoded chat message
                        std::string chat_str = Base64::decode(data["chat"].get<std::string>());

                        std::string message_str = std::string(chat_str.begin(),chat_str.end()- (32));
                        std::string tag_str = std::string(chat_str.end() - 32,chat_str.end());
                        // Convert the decoded string to a vector of unsigned char

                        std::cout << "Encrypted chat message: " << message_str << std::endl;
                        std::cout << "Tag: " << tag_str << std::endl;
                        //Convert tag and ciphertext to bytes from hex
                        std::vector<unsigned char> ciphertext = hexToBytes(chat_str);

                        if (ciphertext.size() < 16){
                            std::cerr << "Ciphertext is too small to contain tag" << std::endl;
                            return "";
                        }
                        // Split the ciphertext: last 16 bytes are the tag, the rest is the actual ciphertext
                        std::vector<unsigned char> tag = hexToBytes(tag_str);
                        std::vector<unsigned char> actual_ciphertext = hexToBytes(message_str);
                        // Decrypt the message
                        std::vector<unsigned char> decrypted_text;
                        if (!aes_gcm_decrypt(actual_ciphertext, key, iv, tag, decrypted_text)) {
                            std::cerr << "\nDecryption failed!: AES" << std::endl;
                            return "";
                        }

                        // Convert the decrypted text back to string
                        std::string decrypted_message(decrypted_text.begin(), decrypted_text.end());
                        return decrypted_message;
                    } else {
                        std::cerr << "Chat is null, cannot decrypt" << std::endl;
                        return "";
                    }
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

