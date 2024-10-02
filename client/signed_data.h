#ifndef SIGNED_DATA_H
#define SIGNED_DATA_H
#include <string>
#include <vector>
#include <openssl/pem.h>
#include "websocket_endpoint.h"
#include <nlohmann/json.hpp>
#include "client_signature.h"
#include "client_key_gen.h"

class SignedData{
    public:
        void static sendSignedMessage(std::string data, EVP_PKEY * private_key, websocket_endpoint* endpoint, int id, int counter);
        std::string static decryptSignedMessage(std::string data, EVP_PKEY * private_key);
};
#endif