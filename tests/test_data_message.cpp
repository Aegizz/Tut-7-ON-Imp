#include "../client/DataMessage.h"
#include "../client/client_key_gen.h"
#include "hexToBytes.h"
#include <iostream>

int numRecipients=5;

int main(){
    // Initialise vectors for test script
    std::vector<EVP_PKEY*> public_keys;
    std::vector<EVP_PKEY*> private_keys;
    std::vector<std::string> server_addresses = {"127.0.0.1:9002","127.0.0.1:9003","127.0.0.1:9004"};

    // Generate RSA keys for recipients of data message
    for(int i=0; i<numRecipients; i++){
        // Generate RSA keys (public and private)
        if (Client_Key_Gen::key_gen()) {
            std::cerr << "Key generation failed!" << std::endl;
            return 1;
        }

        // Load the public key from the file
        EVP_PKEY *pubKey = Client_Key_Gen::loadPublicKey("public_key.pem");
        if (!pubKey) {
            std::cerr << "Failed to load public key!" << std::endl;
            return 1;
        }

        // Load the private key from the file
        EVP_PKEY *privKey = Client_Key_Gen::loadPrivateKey("private_key.pem");
        if (!privKey) {
            std::cerr << "Failed to load private key!" << std::endl;
            return 1;
        }

        public_keys.push_back(pubKey);
        private_keys.push_back(privKey);
    }
    std::cout << "Generated Keys" << std::endl;

    // Generate and print the data message
    std::string dataMsg = DataMessage::generateDataMessage("Hello world!", public_keys, server_addresses);
    std::cout << dataMsg << std::endl;

    // Parse the JSON object received in the message
    nlohmann::json dataJSON = nlohmann::json::parse(dataMsg);

    // Decode cipher text and IV from Base64 into hex, then convert from hex to bytes
    std::string ciphertextHex = Base64::decode(dataJSON["chat"]);
    std::string ciphertextString = hexToBytesString(ciphertextHex);
    std::string ivHex = Base64::decode(dataJSON["iv"]);
    std::string ivString = hexToBytesString(ivHex);

    // Get tag by taking last 16 (specified by AES_GCM_TAG_SIZE) characters of ciphertext in byte form
    std::string tagString = ciphertextString.substr(ciphertextString.length() - AES_GCM_TAG_SIZE);
    // Remove tag from cipher text string to leave actual ciphertext
    ciphertextString = ciphertextString.substr(0, ciphertextString.length() - AES_GCM_TAG_SIZE);

    // For each recipient
    for(int i=0; i<numRecipients; i++){
        // Decode the encrypted symmetric key from Base64 into hex and cast to unsigned char*
        std::string keyStringRSAHex = Base64::decode(dataJSON["symm_keys"][i]);
        const unsigned char* keyStringRSAPointer = reinterpret_cast<const unsigned char *>(keyStringRSAHex.c_str());

        // Decrypt using recipients RSA private key
        unsigned char* decryptedKey = nullptr;
        size_t decryptedKeyLen = (size_t)Client_Key_Gen::rsaDecrypt(private_keys[i], keyStringRSAPointer, keyStringRSAHex.size(), &decryptedKey);

        // Convert produced decrypted unsigned char* to string 
        std::string keyStringHex;
        for(int i=0; i<(int)decryptedKeyLen; i++){
            keyStringHex.push_back(decryptedKey[i]);
        }

        // Convert hex to bytes and convert to unsigned char vector to be used in AES-GCM decryption
        std::string keyString = hexToBytesString(keyStringHex);
        std::vector<unsigned char> key(keyString.begin(), keyString.end());

        // Check if key is the right size
        if (keyString.size() != AES_GCM_KEY_SIZE) {
            std::cerr << "Decryption of the symmetric key failed!" << std::endl;
            return 1;
        }

        OPENSSL_free(decryptedKey);  // Free allocated memory after use
        
        // Convert parameters for AES-GCM decryption to unsigned char vectors 
        std::vector<unsigned char> ciphertext(ciphertextString.begin(), ciphertextString.end());
        std::vector<unsigned char> iv(ivString.begin(), ivString.end());
        std::vector<unsigned char> tag(tagString.begin(), tagString.end());
        std::vector<unsigned char> decrypted_text;

        // Decrypt using AES-GCM and handle any errors
        if (AESGCM::aes_gcm_decrypt(ciphertext, key, iv, tag, decrypted_text)) {
            std::string decrypted_str(decrypted_text.begin(), decrypted_text.end());
            std::cout << "Decryption successful!\nDecrypted text: " << decrypted_str << std::endl;
        } else {
            std::cerr << "Decryption failed!" << std::endl;
            return 1;
        }
    }
    
    // Free memory after use
    for(int i=0; i<numRecipients; i++){
        EVP_PKEY_free(public_keys.at(i));
        EVP_PKEY_free(private_keys.at(i));
    }

    return 0;
}