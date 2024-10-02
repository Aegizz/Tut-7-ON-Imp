#ifndef DATA_MESSAGE_H
#define DATA_MESSAGE_H

#include <string>
#include <vector>
#include "aes_encrypt.h"
#include "client_key_gen.h"
#include "base64.h"
#include <nlohmann/json.hpp>


std::string bytesToHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    for (unsigned char byte : data) {
        oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
    }
    return oss.str();
}



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
            std::cout << "Key: " << bytesToHex(key) << std::endl;
            std::vector<unsigned char> char_text(text.begin(), text.end());
            std::vector<unsigned char> encrypted_text, iv(AES_GCM_IV_SIZE), tag(AES_GCM_TAG_SIZE);

            // Encrypt the plaintext
            if (!aes_gcm_encrypt(char_text, key, encrypted_text, iv, tag)) {
                std::cerr << "Encryption failed" << std::endl;
                return "";
            }

            std::string tagHex = bytesToHex(tag);
            std::cout << "Tag: " << tagHex << std::endl;
            std::string encrypted_string = bytesToHex(encrypted_text);
            std::cout << "Encrypted chat message: " << encrypted_string << std::endl;

            data["chat"] = Base64::encode(encrypted_string + tagHex);
            std::cout << "iv: "<< bytesToHex(iv) << std::endl;
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
            std::cout << "Returning created output as string" << std::endl;
            return data.dump();


        }
};

#endif