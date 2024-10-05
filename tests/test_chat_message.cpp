#include "../client/ChatMessage.h"
#include <iostream>

int main() {
    // Define participants and generate chat message
    std::vector<std::string> participants1 = {"ABCDEF", "GHIJKL"};
    std::string generatedMessage1 = ChatMessage::generateChatMessage("Hello world!", participants1);

    // Output result
    std::cout << "Generated Message: " << generatedMessage1 << std::endl;

    // Define participants and generate chat message
    std::vector<std::string> participants2 = {"MNOPQR", "GHIJKL"};
    std::string generatedMessage2 = ChatMessage::generateChatMessage("Goodbye cruel world!", participants2);

    // Output result
    std::cout << "Generated Message: " << generatedMessage2 << std::endl;

    return 0;
}