#include "../client/client_key_gen.cpp"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>  // For BIGNUM
#include <openssl/bio.h>
#include <openssl/err.h>
#include <cstdio>
#include <cstring>
#include <iostream>

int main() {
    // Load the public and private keys
    if(key_gen())
        return 1;
    
    EVP_PKEY* privateKey = loadPrivateKey("private_key.pem");
    EVP_PKEY* publicKey = loadPublicKey("public_key.pem");

    if (!privateKey || !publicKey) {
        std::cerr << "Error loading keys." << std::endl;
        return 1;
    }

    // Original message
    const unsigned char* message = (const unsigned char*)"Hello, RSA Encryption!";
    size_t message_len = strlen((const char*)message);

    // Encrypt the message using the public key
    unsigned char* encrypted = nullptr;
    int encrypted_len = rsaEncrypt(publicKey, message, message_len, &encrypted);

    std::cout << "Encrypted message length: " << encrypted_len << std::endl;

    // Decrypt the message using the private key
    unsigned char* decrypted = nullptr;
    int decrypted_len = rsaDecrypt(privateKey, encrypted, encrypted_len, &decrypted);

    std::cout << "Decrypted message length: " << decrypted_len << std::endl;

    // Print the decrypted message
    std::cout << "Decrypted message: " << decrypted << std::endl;

    // Clean up
    EVP_PKEY_free(privateKey);
    EVP_PKEY_free(publicKey);
    OPENSSL_free(encrypted);
    OPENSSL_free(decrypted);

    return 0;
}