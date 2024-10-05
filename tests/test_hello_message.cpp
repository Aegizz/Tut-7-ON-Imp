#include "../client/HelloMessage.h"
#include "../client/client_key_gen.h"
#include <string>


int main(){
    EVP_PKEY * pubKey = Client_Key_Gen::loadPublicKey("public_key.pem");
    std::string helloMessage = HelloMessage::generateHelloMessage(pubKey);
    if (helloMessage == ""){
        return -1;
    }
    std::cout << helloMessage << std::endl;    
    return 0;
}