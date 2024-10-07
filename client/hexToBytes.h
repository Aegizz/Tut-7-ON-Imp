#ifndef HEXTOBYTES_H
#define HEXTOBYTES_H

#include <vector>
#include <string>

// Declaration of functions
std::vector<unsigned char> hexToBytes(const std::string& hex);
std::string hexToBytesString(const std::string& hex);
std::string bytesToHex(const std::vector<unsigned char>& data);

#endif  // HEXTOBYTES_H