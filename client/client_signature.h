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
    public:
        static std::string generateSignature(std::string message, EVP_PKEY * private_key, std::string counter);
        static bool verifySignature(std::string encrypted_signature, std::string decrypted_message, EVP_PKEY* publicKey);
};

#endif