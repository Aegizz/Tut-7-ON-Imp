#include "../client/client_signature.h"
#include "../client/client_key_gen.h"
#include "../client/base64.h"

int main(){
    if (key_gen()){
        return 1;
    }
    /* Load RSA key pair for generating signature*/
    EVP_PKEY * key = loadPublicKey("private_key.pem");

    /* Load signature and generate it for testing*/
    std::string signature = ClientSignature::generateSignature("test",key,"12345");
    std::cout << signature << std::endl;
    /*Encode result in base64*/

    std::string base64signature = Base64::encode(signature);
    std::cout << base64signature << std::endl;
}