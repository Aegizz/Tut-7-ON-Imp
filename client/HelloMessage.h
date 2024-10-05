#ifndef HELLOMESSAGE_H
#define HELLOMESSAGE_H
#include <string>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <iostream>

class HelloMessage{
    public:
        static std::string generateHelloMessage(EVP_PKEY * publicKey){
            nlohmann::json data;
            data["type"] = "hello";
            BIO * bio = BIO_new(BIO_s_mem());
            if (!PEM_write_bio_PUBKEY(bio, publicKey)){
                BIO_free(bio);
                std::cerr << "Failed to write public key" << std::endl;
                return "";
            }
            char * pemKey = nullptr;
            long pemLen = BIO_get_mem_data(bio, &pemKey);
            std::string publicKeyStr(pemKey, pemLen);
            BIO_free(bio);
            data["public_key"] = publicKeyStr;
            return data.dump();
        }
};

#endif