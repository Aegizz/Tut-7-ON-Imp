#include "client_signature.h"
#include "client_key_gen.h"
#include "Sha256Hash.h"
ClientSignature::ClientSignature(){

};
ClientSignature::ClientSignature(std::string message, EVP_PKEY * privateKey, std::string counter){
    private_key = privateKey;
    signature = generateSignature(message, private_key, counter);
}
std::string ClientSignature::generateSignature(std::string message, EVP_PKEY * private_key, std::string counter){
    std::string combinedString = message + counter;
    unsigned char ** encrypted;
    const unsigned char * combinedMessage = reinterpret_cast<const unsigned char*>(combinedString.c_str());
    int encrypted_length = rsaEncrypt(private_key, combinedMessage, combinedString.length(), encrypted);
    std::string rsa_string = std::string(reinterpret_cast<char*>(encrypted));
    std::string encrypted_string = Sha256Hash::hashStringSha256(rsa_string);
    return encrypted_string;
}

