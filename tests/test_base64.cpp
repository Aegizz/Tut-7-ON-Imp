#include "../client/base64.h"

int main() {
    std::string original = "Hello, World!";
    std::string encoded = Base64::encode(original);
    std::string decoded = Base64::decode(encoded);

    std::cout << "Original: " << original << std::endl;
    std::cout << "Encoded: " << encoded << std::endl;
    std::cout << "Decoded: " << decoded << std::endl;

    return 0;
}