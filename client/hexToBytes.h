#ifndef HEXTOBYTES_H
#define HEXTOBYTES_H

#include <iomanip>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
/*
    Converts hex To Bytes from the base64 encoding to be passed to the decryption function.
*/
std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    size_t length = hex.length();

    // Ensure the length of hex string is even
    if (length % 2 != 0) {
        throw std::invalid_argument("Hex string must have an even length.");
    }

    for (size_t i = 0; i < length; i += 2) {
        std::string byteString = hex.substr(i, 2); // Get two characters
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
};

/* Converts hex to bytes when decoding from base64 
   A function in signed_data.cpp causes issues with test_signed_data.cpp if this function is called hexToBytes*/
std::string hexToBytesString(const std::string& hex) {
    std::string bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);  // Take two characters (1 byte)
        unsigned char byte = (unsigned char) strtol(byteString.c_str(), nullptr, 16);  // Convert hex to byte
        bytes.push_back(byte);
    }
    return bytes;
}

/* Converts bytes to hex to be passed to base64 encoding */
std::string bytesToHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    for (unsigned char byte : data) {
        oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
    }
    return oss.str();
}
#endif