#ifndef DATA_MESSAGE_H
#define DATA_MESSAGE_H

#include <string>
#include <vector>
#include "aes_encrypt.h"
#include "client_key_gen.h"
#include "base64.h"
#include <nlohmann/json.hpp>

class DataMessage{
    public:
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
            if (!aes_gcm_encrypt(char_text, key, encrypted_text, iv, tag)) {
                std::cerr << "Encryption failed" << std::endl;
                return "";
            } else {
                std::cout << "Encryption successful!" << std::endl;
            }
            std::cout << "Adding IV" << std::endl;
            data["iv"] = std::string(iv.begin(), iv.end());

            std::cout << "Adding symmetric keys" << std::endl;

            // Encrypt the symmetric key with each public key
            for (EVP_PKEY* public_key : public_keys) {
                unsigned char* symm_key = nullptr;
                size_t symm_key_len = 0;  // Initialize the length

                // Check if encryption is successful
                if (Client_Key_Gen::rsaEncrypt(public_key, key.data(), key.size(), &symm_key)) {
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