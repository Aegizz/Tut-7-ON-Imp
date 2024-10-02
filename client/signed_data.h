#ifndef SIGNED_DATA_H
#define SIGNED_DATA_H
#include <string>
#include <openssl/pem.h>
#include "websocket_endpoint.h"

class SignedData{
    public:
        void sendSignedMessage(std::string data, EVP_PKEY * private_key, websocket_endpoint* endpoint, int id);
};
#endif