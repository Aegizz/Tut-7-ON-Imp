# API Reference for using the Client Function Library


## Use the MessageGenerator class for creating any kind of client message.
  This class contains all messages to be created by a client.
  ```cpp
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
            static std::string chatMessage(std::string message, EVP_PKEY * your_private_key, EVP_PKEY * your_public_key, std::vector<EVP_PKEY*> their_public_keys, std::vector<std::string> destination_servers_vector, int client_id, int server_id, int counter);
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
                    "signature": "<Base64 signature of data + counter>"
                }
            */
          static std::string publicChatMessage(std::string message, EVP_PKEY * your_private_key, EVP_PKEY * your_public_key,int counter);

          /*
                Client list
                To retrieve a list of all currently connected clients on all servers. Your server will send a JSON response. This does not follow the data structure.

                {
                    "type": "client_list_request",
                }
                This is NOT signed and does NOT follow the data format.
          */
            static std::string clientListRequestMessage();
    }
```


## This is the functiuons that the MessageGenerator calls and what they do
  You do not need to use these functions just the MessageGenerator class.


### Sending a signed_data chat

  Send signed data with signed_data.h

```cpp
    /* 
        Will send a signed message over the specified endpoint and id 
    */
    void SignedData::sendSignedMessage(std::string data, EVP_PKEY * private_key, websocket_endpoint* endpoint, int id, int counter);
    /* 
        Iterates over a signed message and attempts to decrypt the aes key with it's own private key
        Returns the decrypted message.
        Function to decrypt a signed messsage's content, does not verify the reciever or check counter
        Ensure the entire message is provided.
    */
    std::string SignedData::decryptSignedMessage(std::string data, EVP_PKEY * private_key);

```

### Generate a client hello

  This message is sent when first connection to a server to establish your public key.
  ```cpp
            /* Used for generating server hello messages to server public key to a server to be sent to clients*/
          std::string HelloMessage::generateHelloMessage(EVP_PKEY * publicKey);
  ```


### Generating a chat message 

  Generate a chat message

  ```cpp
    /*
        Function to take a user message and a vector array of chat participants and convert it to a json string for encrypting in DataMessage
    */
    std::string ChatMessage::generateChatMessage(std::string plaintext_message, std::vector<std::string> chat_participants);
  ```


### Generating a signed_data message
  
  Generate a signed_data messaage

  ```cpp
    /* 
        Used for creating the data in a chat message, currently missing client-info and time-to-die
        Returns the resultant string to be provided to the websocket or to be signed in signed_data function.
    */
    std::string DataMessage::generateDataMessage(std::string text, std::vector<EVP_PKEY*> public_keys, std::vector<std::string> server_addresses);
  ```

### Generate a fingerprint

  Generate a fingerprint using your public key.
  
  ```cpp
  /*
    Used to create a fingerprint used in public chat hello messages
  */
  std::string Fingerprint::generateFingerprint(EVP_PKEY * publicKey)

  ```
### Generate a Public Chat Message
  
  Generate a Public Chat Message for sending public messages
  ```cpp
    /* 
      Used for generating public chat messages for sending a public chat message,
      returns the simple json formatted string
      feel like this is vulnerable to someone resending a request as afaik this is not to be signed?
    */
    std::string PublicChatMessage::generatePublicChatMessage(std::string message, EVP_PKEY * publicKey);
  ```