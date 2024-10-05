#include "../client/PublicChatMessage.h"
#include "../client/client_key_gen.h"
#include <iostream>
#include <string>
#include <openssl/pem.h>

int main(){
    EVP_PKEY * pubKey = Client_Key_Gen::loadPublicKey("public_key.pem");
    std::string pubChatMessage = PublicChatMessage::generatePublicChatMessage("Hello world!", pubKey);
    if (pubChatMessage == ""){
        std::cerr << "generatePublicChatMessage returned empty string" << std::endl;
        return -1;
    }   
    std::cout << "Generated public chat data: " << pubChatMessage << std::endl;
    return 0;
}