#include "../client/HelloMessage.h"
#include "../client/client_key_gen.h"
#include <string>


int main(){
    EVP_PKEY * pubKey = Client_Key_Gen::loadPublicKey("public_key.pem");
    std::string helloMessage = HelloMessage::generateHelloMessage(pubKey);
    std::cout << helloMessage << std::endl;    
}