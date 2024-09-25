#ifndef AES_ENCRYPT_H
#define AES_ENCRYPT_H

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <vector>
#include <cstring>


// Constants
const int AES_GCM_KEY_SIZE = 32;  // 256-bit key
const int AES_GCM_IV_SIZE = 16;   // 128-bit IV
const int AES_GCM_TAG_SIZE = 16;  // 128-bit authentication tag

bool aes_gcm_encrypt(const std::vector<unsigned char>& plaintext, const std::vector<unsigned char>& key,
                     std::vector<unsigned char>& ciphertext, std::vector<unsigned char>& iv, std::vector<unsigned char>& tag);

bool aes_gcm_decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& key,
                     const std::vector<unsigned char>& iv, const std::vector<unsigned char>& tag, std::vector<unsigned char>& decrypted_text);

#endif
