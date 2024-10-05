#ifndef PUBLICCHATMESSAGE_H
#define PUBLICCHATMESSAGE_H

#include <string>
#include <openssl/pem.h>
#include "Fingerprint.h"
#include <nlohmann/json.hpp>
class PublicChatMessage{
    public:
        /* 
            Used for generating public chat messages for sending a public chat message,
            returns the simple json formatted string
            feel like this is vulnerable to someone resending a request as afaik this is not to be signed?
        */
        static std::string generatePublicChatMessage(std::string message, EVP_PKEY * publicKey){
            nlohmann::json data;
            data["type"] = "public_chat";
            data["sender"] = Fingerprint::generateFingerprint(publicKey);
            data["message"] = message;

            return data.dump();
        }
};

#endif