#include "../client/Sha256Hash.h"
#include <string>


int main(){
    Sha256Hash * test_hash;

    test_hash = new Sha256Hash("test");
    if (test_hash->getHash() != "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08"){
        std::cout << test_hash->getHash() << std::endl;

        std::cout << "Hash does not match, run into some error";
        std::cerr << "Hash does not match, run into some error";
        return -1;
    } else {
        std::cout << "Hash is correct!\n" << std::endl;
    }
    return 0;
}
