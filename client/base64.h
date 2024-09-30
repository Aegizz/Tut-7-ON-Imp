#include <iostream>
#include <string>
#include <vector>

class Base64 {
public:
    // Encode the input string to Base64
    static std::string encode(const std::string &input);

    // Decode the input Base64 string
    static std::string decode(const std::string &encoded_string);
};


