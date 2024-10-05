#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "client_key_gen.h"
#include "Sha256Hash.h"
#include "base64.h"
#include <string>


class Fingerprint{
    public:
        /* Used to create a fingerprint used in public chat hello messages */
        static std::string generateFingerprint(EVP_PKEY * publicKey){
            BIO * bio = BIO_new(BIO_s_mem());
            if (!PEM_write_bio_PUBKEY(bio, publicKey)){
                BIO_free(bio);
                std::cerr << "Failed to write public key" << std::endl;
                return "";
            }
            char * pemKey = nullptr;
            long pemLen = BIO_get_mem_data(bio, &pemKey);
            std::string publicKeyStr(pemKey, pemLen);
            std::string hashedKey = Sha256Hash::hashStringSha256(publicKeyStr);
            std::string encodedHash = Base64::encode(hashedKey);

            return encodedHash;
        }
};
#endif