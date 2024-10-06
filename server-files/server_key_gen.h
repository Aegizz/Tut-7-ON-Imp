#ifndef SERVER_KEY_GEN_H
#define SERVER_KEY_GEN_H

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>  // For BIGNUM
#include <openssl/bio.h>
#include <openssl/err.h>
#include <cstdio>
#include <cstring>
#include <iostream>


class Server_Key_Gen{
    private:
        static void handleErrors();
    public:
        static int key_gen(int server_id);
        static EVP_PKEY * loadPrivateKey(const char* filename);
        static EVP_PKEY * loadPublicKey(const char* filename);
        static int rsaEncrypt(EVP_PKEY* pubKey, const unsigned char* plaintext, size_t plaintext_len, unsigned char** encrypted);
        static int rsaDecrypt(EVP_PKEY* privKey, const unsigned char* encrypted, size_t encrypted_len, unsigned char** decrypted);
        static int rsaSign(EVP_PKEY* privKey, const unsigned char* data, size_t data_len, unsigned char** signature);
        static int rsaVerify(EVP_PKEY* pubKey, const unsigned char* data, size_t data_len, const unsigned char* signature, size_t signature_len);
        static EVP_PKEY* stringToPEM(std::string pKey);
};
#endif