#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <vector>
#include <cstring>
#include "../client/aes_encrypt.h"


int main() {
    // Sample 256-bit (32 bytes) key
    std::vector<unsigned char> key(AES_GCM_KEY_SIZE);
    RAND_bytes(key.data(), AES_GCM_KEY_SIZE);

    // Sample plaintext
    std::string plaintext_str = "This is a test message for AES-GCM encryption!";
    std::vector<unsigned char> plaintext(plaintext_str.begin(), plaintext_str.end());

    std::vector<unsigned char> ciphertext, iv, tag, decrypted_text;

    // Encrypt the plaintext
    if (aes_gcm_encrypt(plaintext, key, ciphertext, iv, tag)) {
        std::cout << "Encryption successful!" << std::endl;
    } else {
        std::cerr << "Encryption failed!" << std::endl;
        return 1;
    }
    //print cypher text
    // i have no clue why it wants a static cast to an int there otherwise they were compiler warnings
    // beats me
    for (int i = 0; i < static_cast<int>(ciphertext.size()); i++){
        std::cout << ciphertext[i] << " ";
    }
    std::cout << std::endl;

    // Decrypt the ciphertext
    if (aes_gcm_decrypt(ciphertext, key, iv, tag, decrypted_text)) {
        std::string decrypted_str(decrypted_text.begin(), decrypted_text.end());
        std::cout << "Decryption successful!\nDecrypted text: " << decrypted_str << std::endl;
    } else {
        std::cerr << "Decryption failed!" << std::endl;
        return 1;
    }

    return 0;
}