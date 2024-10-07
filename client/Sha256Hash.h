#ifndef SHA256_HASH_H
#define SHA256_HASH_H
#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <sstream>
#include <iomanip>
#include <iostream>

class Sha256Hash{
    public:
        /* 
            Returns the Sha256 Hash of a given string input.
        */
        static std::string hashStringSha256(const std::string &input);
};

#endif