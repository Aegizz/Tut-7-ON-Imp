#ifndef CLIENT_SIGNATURE_H
#define CLIENT_SIGNATURE_H
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>  // For BIGNUM
#include <openssl/bio.h>
#include <openssl/err.h>
#include <string>

class ClientSignature{
    private:
        EVP_PKEY * private_key;
        std::string signature;
    public:
        ClientSignature();
        ClientSignature(std::string message, EVP_PKEY * private_key, std::string counter);
        static std::string generateSignature(std::string message, EVP_PKEY * private_key, std::string counter);
        std::string getSignature();
};

#endif