#include "../client/Fingerprint.h"
#include "../client/client_key_gen.h"
#include "../client/Sha256Hash.h"
#include "../client/base64.h"
#include <openssl/pem.h>
#include <string>
int main(){
    EVP_PKEY * pubKey = Client_Key_Gen::loadPublicKey("tests/test-keys/public_key0.pem");
    std::string finger = Fingerprint::generateFingerprint(pubKey);
    if (finger == ""){
        std::cerr << "Fingerprint failed" << std::endl;
        return -1;
    }
    std::cout << "Calculated fingerprint from public key: " << finger << std::endl;
    //Generate pubkey string
    BIO * bio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PUBKEY(bio, pubKey)){
        BIO_free(bio);
        std::cerr << "Failed to write public key" << std::endl;
        return -1;
    }
    char * pemKey = nullptr;
    long pemLen = BIO_get_mem_data(bio, &pemKey);
    std::string publicKeyStr(pemKey, pemLen);
    //hash string and compare base64 decode output fromg generateFingerprint
    std::string hash = Sha256Hash::hashStringSha256(publicKeyStr);
    std::string hashedString = Base64::encode(hash);
    if (hashedString != finger){
        return -1;
    }
    std::cout << "Key and fingerprint match!" << std::endl;

    return 0;
}