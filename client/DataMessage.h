#ifndef DATA_MESSAGE_H
#define DATA_MESSAGE_H

#include <string>
#include <vector>
#include <sstream>
#include "aes_encrypt.h"
#include "client_key_gen.h"
#include "base64.h"
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>

/* Converts bytes to hex to be passed to base64 encoding */
std::string bytesToHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    for (unsigned char byte : data) {
        oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
    }
    return oss.str();
}

/* Converts hex to bytes when decoding from base64 
   A function in signed_data.cpp causes issues with test_signed_data.cpp if this function is called hexToBytes*/
std::string hexToBytesString(const std::string& hex) {
    std::string bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);  // Take two characters (1 byte)
        unsigned char byte = (unsigned char) strtol(byteString.c_str(), nullptr, 16);  // Convert hex to byte
        bytes.push_back(byte);
    }
    return bytes;
}

class DataMessage{
    public:
        /* 
            Used for creating the data in a chat message, currently missing client-info and time-to-die
            Returns the resultant string to be provided to the websocket or to be signed in signed_data function.
        */
        static std::string generateDataMessage(std::string text, std::vector<EVP_PKEY*> public_keys, std::vector<std::string> server_addresses){
            nlohmann::json data;
            data["type"] = "chat";
            data["destination_servers"] = server_addresses;
            // Generate random AES key
            std::vector<unsigned char> key(AES_GCM_KEY_SIZE);
            if (!RAND_bytes(key.data(), AES_GCM_KEY_SIZE)) {
                std::cerr << "Error generating random key." << std::endl;
                return "";
            }

            std::vector<unsigned char> char_text(text.begin(), text.end());
            std::vector<unsigned char> encrypted_text, iv(AES_GCM_IV_SIZE), tag(AES_GCM_TAG_SIZE);

            // Encrypt the plaintext
            if (!AESGCM::aes_gcm_encrypt(char_text, key, encrypted_text, iv, tag)) {
                std::cerr << "Encryption failed" << std::endl;
                return "";
            }

            std::string tagHex = bytesToHex(tag);
            std::string encrypted_string = bytesToHex(encrypted_text);

            data["chat"] = Base64::encode(encrypted_string + tagHex);
            data["iv"] = Base64::encode(bytesToHex(iv));

            std::string base64Key = bytesToHex(key);

            const unsigned char * key_encoded = reinterpret_cast<const unsigned char *>(base64Key.c_str());
            // Encrypt the symmetric key with each public key
            for (EVP_PKEY* public_key : public_keys) {
                unsigned char* symm_key = nullptr;
                size_t symm_key_len = 0;  // Initialize the length

                // Check if encryption is successful
                if ((symm_key_len = Client_Key_Gen::rsaEncrypt(public_key, key_encoded, base64Key.length(), &symm_key))) {
                    std::string symm_key_string(reinterpret_cast<char*>(symm_key), symm_key_len);
                    data["symm_keys"].push_back(Base64::encode(symm_key_string));
                    OPENSSL_free(symm_key); // Free the allocated memory
                } else {
                    std::cerr << "Error encrypting symmetric key." << std::endl;
                    return "";
                }
            }
            return data.dump();


        }
};

#endif