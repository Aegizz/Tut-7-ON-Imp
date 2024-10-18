#include "../client/HelloMessage.h"
#include "../client/client_key_gen.h"
#include <string>


int main(){
    if(Client_Key_Gen::key_gen(0, false, true)){
        std::cerr << "Key generation failed!" << std::endl;
        return 1;
    }

    EVP_PKEY * pubKey = Client_Key_Gen::loadPublicKey("tests/test-keys/public_key0.pem");
    if(!pubKey){
        std::cerr << "Failed to load public key!" << std::endl;
        return 1;
    }

    std::string helloMessage = HelloMessage::generateHelloMessage(pubKey);
    if (helloMessage == ""){
        return -1;
    }
    std::cout << helloMessage << std::endl;    
    return 0;
}