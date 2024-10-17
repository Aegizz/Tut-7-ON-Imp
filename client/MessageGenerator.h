#ifndef MESSAGE_GENERATOR_H
#define MESSAGE_GENERATOR_H

//Used for handling keys
#include <openssl/evp.h>
//Used for generating strings
#include <string>
//Used for storing vectors of public keys
#include <vector>
//Used for formatting strings to fit json format
#include <nlohmann/json.hpp>


/* 
    This class is used to globally reference a way of generating all kinds of messages for a client.
    All functions should return json formatted strings that should be sent directly over websocket to the server.
*/
class MessageGenerator{
    public:
        /*

            Hello
            This message is sent when first connecting to a server to establish your public key.

                {
                    "data": {
                        "type": "hello",
                        "public_key": "<Exported RSA public key>"
                    }
                }]

            This message is always signed.
            {
                "type": "signed_data",
                "data": {  },
                "counter": 12345,
                "signature": "<Base64 signature of data + counter>"
            }
        */
        static std::string helloMessage(EVP_PKEY * your_private_key ,EVP_PKEY * your_public_key, int counter);
        /*
            Chat
            Sent when a user wants to send a chat message to another user[s]. Chat messages are end-to-end encrypted. Time to death is 1 minute.

            {
                "data": {
                    "type": "chat",
                    "destination_servers": [
                        "<Address of each recipient's destination server>",
                    ],
                    "iv": "<Base64 encoded AES initialisation vector>",
                    "symm_keys": [
                        "<Base64 encoded AES key, encrypted with each recipient's public RSA key>",
                    ],
                    "chat": "<Base64 encoded AES encrypted segment>",
                    "client-info":{
                        "client-id":"<client-id>",
                        "server-id":"<server-id>"
                    },
                    "time-to-die":"UTC-Timestamp"
                }
            }
            Chat format
            {
                "chat": {
                    "participants": [
                        "<Base64 encoded list of fingerprints of participants, starting with sender>",
                    ],
                    "message": "<Plaintext message>"
                }
            }
            This message is always signed.
            {
                "type": "signed_data",
                "data": {  },
                "counter": 12345,
                "signature": "<Base64 signature of data + counter>"
            }
        */
        static std::string chatMessage(std::string message, EVP_PKEY * your_private_key, EVP_PKEY * your_public_key, std::vector<EVP_PKEY*> their_public_keys, std::vector<std::string> destination_servers_vector, int counter, std::string ttd, int client_id=0, int server_id=0);
        /*
        Public chat
        Public chats are not encrypted at all and are broadcasted as plaintext.
            {
                "data": {
                    "type": "public_chat",
                    "sender": "<Fingerprint of sender>",
                    "message": "<Plaintext message>"
                }
            }
            This message is always signed.
            {
                "type": "signed_data",
                "data": {  },
                "counter": 12345,
                "time-to-die":"UTC-Timestamp",
                "signature": "<Base64 signature of data + counter>"
            }
        */
       static std::string publicChatMessage(std::string message, EVP_PKEY * your_private_key, EVP_PKEY * your_public_key, int counter);

       /*
            Client list
            To retrieve a list of all currently connected clients on all servers. Your server will send a JSON response. This does not follow the data structure.

            {
                "type": "client_list_request",
            }
            This is NOT signed and does NOT follow the data format.
       */
        static std::string clientListRequestMessage();

};

#endif