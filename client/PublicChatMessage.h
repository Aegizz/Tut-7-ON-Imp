#ifndef PUBLICCHATMESSAGE_H
#define PUBLICCHATMESSAGE_H

#include <string>
#include "Sha256Hash.h"
#include <openssl/pem.h>

class PublicChatMessage{
    public:
        static std::string generatePublicChatMessage(std::string message, EVP_PKEY * publicKey){
            return "";
        }
};

#endif