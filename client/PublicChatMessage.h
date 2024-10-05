#ifndef PUBLICCHATMESSAGE_H
#define PUBLICCHATMESSAGE_H

#include <string>
#include <openssl/pem.h>
#include "Fingerprint.h"
#include <nlohmann/json.hpp>
class PublicChatMessage{
    public:
        static std::string generatePublicChatMessage(std::string message, EVP_PKEY * publicKey){
            nlohmann::json data;
            data["type"] = "public_chat";
            data["sender"] = Fingerprint::generateFingerprint(publicKey);
            data["message"] = message;

            return data.dump();
        }
};

#endif