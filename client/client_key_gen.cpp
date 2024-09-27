#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>  // For BIGNUM
#include <openssl/bio.h>
#include <openssl/err.h>
#include <cstdio>
#include <iostream>

void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

int gen_keys() {
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
