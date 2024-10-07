#include "hexToBytes.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

// Definitions of functions
std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    size_t length = hex.length();

    if (length % 2 != 0) {
        throw std::invalid_argument("Hex string must have an even length.");
    }

    for (size_t i = 0; i < length; i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}

std::string hexToBytesString(const std::string& hex) {
    std::string bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char) strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytesToHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    for (unsigned char byte : data) {
        oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
    }
    return oss.str();
}
