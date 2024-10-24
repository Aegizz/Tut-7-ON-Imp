#include "../client/client_signature.h"
#include "../client/client_key_gen.h"
#include "../client/base64.h"
#include <openssl/evp.h>
#include <iostream>

int main() {
    // Generate RSA keys (public and private)
    if (Client_Key_Gen::key_gen(0, "tests")) {
        std::cerr << "Key generation failed!" << std::endl;
        return 1;
    }

    // Load the private key from the file
    EVP_PKEY *privKey = Client_Key_Gen::loadPrivateKey("tests/test-keys/private_key0.pem");
    if (!privKey) {
        std::cerr << "Failed to load private key!" << std::endl;
        return 1;
    }

    // Load the public key from the file
    EVP_PKEY *pubKey = Client_Key_Gen::loadPublicKey("tests/test-keys/public_key0.pem");
    if (!pubKey) {
        std::cerr << "Failed to load public key!" << std::endl;
        EVP_PKEY_free(privKey);  // Clean up before exiting
        return 1;
    }

    std::cout << "Loaded Key" << std::endl;

    // Generate a signature for the message "test" using the private key
    std::string message = "test";
    std::string signature = ClientSignature::generateSignature(message, privKey, "12345");
    if (signature.empty()) {
        std::cerr << "Failed to generate signature!" << std::endl;
        EVP_PKEY_free(privKey);
        EVP_PKEY_free(pubKey);
        return -1;
    }

    std::cout << "Generated signature: " << signature << std::endl;

    // Verify the generated signature using the public key
    bool isValid = ClientSignature::verifySignature(signature, message + "12345", pubKey);
    if (isValid) {
        std::cout << "Signature verification succeeded!" << std::endl;
    } else {
        std::cerr << "Signature verification failed!" << std::endl;
        EVP_PKEY_free(privKey);
        EVP_PKEY_free(pubKey);
        return -1;
    }

    // Clean up OpenSSL key structures
    EVP_PKEY_free(privKey);
    EVP_PKEY_free(pubKey);

    return 0;
}
