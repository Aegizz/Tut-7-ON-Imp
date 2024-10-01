#include "client_signature.h"
#include "client_key_gen.h"
#include "Sha256Hash.h"
#include "base64.h"
ClientSignature::ClientSignature(){

};
ClientSignature::ClientSignature(std::string message, EVP_PKEY * privateKey, std::string counter){
    private_key = privateKey;
    signature = generateSignature(message, private_key, counter);
}
std::string ClientSignature::generateSignature(std::string message, EVP_PKEY *private_key, std::string counter) {
    // Combine the message and counter, then hash the result
    std::string combinedString = message + counter;
    std::string hashed_string = Sha256Hash::hashStringSha256(combinedString);

    // Prepare the buffer for encryption
    unsigned char *encrypted = nullptr; // Single pointer to hold encrypted data
    const unsigned char *combinedMessage = reinterpret_cast<const unsigned char*>(hashed_string.c_str());

    // Encrypt the hashed string using rsaEncrypt
    int encrypted_length = rsaEncrypt(private_key, combinedMessage, hashed_string.length(), &encrypted);

    // If encryption failed, return an empty string (or handle the error accordingly)
    if (encrypted_length <= 0 || encrypted == nullptr) {
        std::cerr << "Encryption failed." << std::endl;
        return "";
    }

    // Convert the encrypted data to a std::string
    std::string rsa_string(reinterpret_cast<char*>(encrypted), encrypted_length);

    // Base64 encode the encrypted string to create the signature
    std::string base64signature = Base64::encode(rsa_string);

    // Free the encrypted buffer after use
    OPENSSL_free(encrypted);

    return base64signature;
}

std::string ClientSignature::decryptSignature(std::string encrypted_signature, EVP_PKEY *public_key) {
    // Decode the base64 encoded signature
    std::string decoded_signature = Base64::decode(encrypted_signature);
    
    // Convert the decoded signature to an unsigned char array
    const unsigned char *combinedSignature = reinterpret_cast<const unsigned char*>(decoded_signature.c_str());
    
    // Prepare a pointer for the decrypted data
    unsigned char *decrypted = nullptr;

    // Perform the RSA decryption
    int decrypted_len = rsaDecrypt(public_key, combinedSignature, decoded_signature.length(), &decrypted);

    // If decryption failed, handle the error
    if (decrypted_len <= 0 || decrypted == nullptr) {
        std::cerr << "Decryption failed." << std::endl;
        fprintf(stderr, "Decryption failed\n");
        return "";
    }

    // Convert the decrypted data to a std::string
    std::string decrypted_string(reinterpret_cast<char*>(decrypted), decrypted_len);

    // Free the allocated memory for the decrypted data
    OPENSSL_free(decrypted);

    return decrypted_string;
}


std::string ClientSignature::getSignature(){
    return signature;
}

bool ClientSignature::verifySignature(std::string encrypted_signature, std::string decrypted_message, EVP_PKEY* publicKey){
    std::string decrypted_signature = decryptSignature(encrypted_signature, publicKey);
    std::string hashed_message = Sha256Hash::hashStringSha256(decrypted_message);
    return (hashed_message == decrypted_signature);
}
