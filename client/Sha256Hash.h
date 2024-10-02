#ifndef SHA256_HASH_H
#define SHA256_HASH_H
#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <sstream>
#include <iomanip>
#include <iostream>

class Sha256Hash{
    private:
        std::string input_string;
        std::string hash;

    public:
        Sha256Hash();
        Sha256Hash(std::string input);
        static std::string hashStringSha256(const std::string &input);
        std::string getHash();
};

#endif