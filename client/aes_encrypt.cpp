#include "aes_encrypt.h"


bool aes_gcm_encrypt(const std::vector<unsigned char>& plaintext, const std::vector<unsigned char>& key,
                     std::vector<unsigned char>& ciphertext, std::vector<unsigned char>& iv, std::vector<unsigned char>& tag) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    // Generate a random IV
    iv.resize(AES_GCM_IV_SIZE);
    if (!RAND_bytes(iv.data(), AES_GCM_IV_SIZE)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Initialize encryption context with AES-256-GCM
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Set IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_SIZE, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Initialize key and IV
    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Encrypt the plaintext
    ciphertext.resize(plaintext.size());
    int len;
    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int ciphertext_len = len;

    // Finalize encryption
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    ciphertext.resize(ciphertext_len);

    // Get the authentication tag
    tag.resize(AES_GCM_TAG_SIZE);
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_SIZE, tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool aes_gcm_decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& key,
                     const std::vector<unsigned char>& iv, const std::vector<unsigned char>& tag, std::vector<unsigned char>& decrypted_text) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    // Initialize decryption context with AES-256-GCM
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Set IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_SIZE, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Initialize key and IV
    if (1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Decrypt the ciphertext
    decrypted_text.resize(ciphertext.size());
    int len;
    if (1 != EVP_DecryptUpdate(ctx, decrypted_text.data(), &len, ciphertext.data(), ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int decrypted_len = len;

    // Set expected tag value
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_SIZE, const_cast<unsigned char*>(tag.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Finalize decryption (returns false if tag verification fails)
    if (1 != EVP_DecryptFinal_ex(ctx, decrypted_text.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    decrypted_len += len;

    decrypted_text.resize(decrypted_len);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}
