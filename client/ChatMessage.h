#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ChatMessage{
    public:
        /*Function to take a user message and a vector array of chat participants and convert it to a json string for encrypting in DataMessage*/
        std::string static generateChatMessage(std::string plaintext_message, std::vector<std::string> chat_participants){
            nlohmann::json json_chat;
            json_chat["message"] = plaintext_message;
            for (int i = 0; i < chat_participants.size(); i++){
                json_chat["participants"].push_back(chat_participants[i]);
            }
            //could use datamessage function here to add signed_data information
            return json_chat.dump();
        }
};

#endif
