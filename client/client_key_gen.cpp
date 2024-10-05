#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>  // For BIGNUM
#include <openssl/bio.h>
#include <openssl/err.h>
#include <cstdio>
#include <cstring>
#include <iostream>

void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

int key_gen() {
    // 1. Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // 2. Create RSA key generation context
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) handleErrors();

    // 3. Generate RSA private key (2048 bits)
    if (EVP_PKEY_keygen_init(ctx) <= 0) handleErrors();
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) handleErrors();
    
    // 4. Set public exponent to 65537 (must use BIGNUM for the exponent)
    BIGNUM *e = BN_new();
    if (!e || !BN_set_word(e, RSA_F4)) {  // RSA_F4 is 65537
        std::cerr << "Error setting public exponent\n";
        handleErrors();
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, e) <= 0) handleErrors();
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) handleErrors();

    // 4. Write the private key to private_key.pem (PEM format)
    FILE *private_key_file = fopen("private_key.pem", "wb");
    if (!private_key_file) {
        std::cerr << "Unable to open private_key.pem for writing\n";
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }
    if (!PEM_write_PrivateKey(private_key_file, pkey, NULL, NULL, 0, NULL, NULL)) {
        std::cerr << "Error writing private key\n";
        handleErrors();
    }
    fclose(private_key_file);

    // 5. Write the public key to public_key.pem (SPKI format, PEM)
    FILE *public_key_file = fopen("public_key.pem", "wb");
    if (!public_key_file) {
        std::cerr << "Unable to open public_key.pem for writing\n";
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }
    if (!PEM_write_PUBKEY(public_key_file, pkey)) {
        std::cerr << "Error writing public key\n";
        handleErrors();
    }
    fclose(public_key_file);

    // Clean up
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);

    // 6. Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();

    //std::cout << "Private key and public key (SPKI format) have been saved successfully." << std::endl;
    
    return 0;
}


EVP_PKEY* loadPrivateKey(const char* filename) {
    FILE* keyFile = fopen(filename, "rb");
    if (!keyFile) {
        std::cerr << "Unable to open private key file." << std::endl;
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_PrivateKey(keyFile, NULL, NULL, NULL);
    fclose(keyFile);
    if (!pkey) {
        std::cerr << "Error reading private key." << std::endl;
        handleErrors();
    }
    return pkey;
}

// Function to read a PEM-encoded public key from a file
EVP_PKEY* loadPublicKey(const char* filename) {
    FILE* keyFile = fopen(filename, "rb");
    if (!keyFile) {
        std::cerr << "Unable to open public key file." << std::endl;
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_PUBKEY(keyFile, NULL, NULL, NULL);
    fclose(keyFile);
    if (!pkey) {
        std::cerr << "Error reading public key." << std::endl;
        handleErrors();
    }
    return pkey;
}

// Function to encrypt data using the public key
int rsaEncrypt(EVP_PKEY* pubKey, const unsigned char* plaintext, size_t plaintext_len, unsigned char** encrypted) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubKey, NULL);
    if (!ctx) handleErrors();

    if (EVP_PKEY_encrypt_init(ctx) <= 0) handleErrors();

    // Determine buffer length for the encrypted data
    size_t encrypted_len;
    if (EVP_PKEY_encrypt(ctx, NULL, &encrypted_len, plaintext, plaintext_len) <= 0) handleErrors();

    *encrypted = (unsigned char*)OPENSSL_malloc(encrypted_len);
    if (*encrypted == NULL) handleErrors();

    // Encrypt the data
    if (EVP_PKEY_encrypt(ctx, *encrypted, &encrypted_len, plaintext, plaintext_len) <= 0) handleErrors();

    EVP_PKEY_CTX_free(ctx);
    return encrypted_len;  // Return length of the encrypted data
}

// Function to decrypt data using the private key
int rsaDecrypt(EVP_PKEY* privKey, const unsigned char* encrypted, size_t encrypted_len, unsigned char** decrypted) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privKey, NULL);
    if (!ctx) handleErrors();

    if (EVP_PKEY_decrypt_init(ctx) <= 0) handleErrors();

    // Determine buffer length for the decrypted data
    size_t decrypted_len;
    if (EVP_PKEY_decrypt(ctx, NULL, &decrypted_len, encrypted, encrypted_len) <= 0) handleErrors();

    *decrypted = (unsigned char*)OPENSSL_malloc(decrypted_len);
    if (*decrypted == NULL) handleErrors();

    // Decrypt the data
    if (EVP_PKEY_decrypt(ctx, *decrypted, &decrypted_len, encrypted, encrypted_len) <= 0) handleErrors();

    EVP_PKEY_CTX_free(ctx);
    return decrypted_len;  // Return length of the decrypted data
}
