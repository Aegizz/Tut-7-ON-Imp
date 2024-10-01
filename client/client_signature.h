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
        ClientSignature();
        static std::string generateSignature(std::string message, EVP_PKEY * private_key, std::string counter);
        static bool decryptSignature(std::string encrypted_message, EVP_PKEY * public_key);
        static bool verifySignature(std::string encrypted_signature, std::string decrypted_message, EVP_PKEY* publicKey);
};

#endif