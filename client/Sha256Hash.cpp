#include "Sha256Hash.h"


std::string Sha256Hash::hashStringSha256(const std::string &input){
    // Create a context for the hashing operation
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        // Handle error: failed to create context
        return "";
    }

    // Initialize the context for SHA-256 hashing
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        // Handle error: failed to initialize digest
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // Update the context with the input data
    if (EVP_DigestUpdate(ctx, input.data(), input.size()) != 1) {
        // Handle error: failed to update digest
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // Finalize the hashing operation
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        // Handle error: failed to finalize digest
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // Clean up the context
    EVP_MD_CTX_free(ctx);

    // Convert the hash to a hexadecimal string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}
