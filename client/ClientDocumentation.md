# API Reference for using the Client Function Library



### Sending a signed_data chat

  Send signed data with signed_data.h

```cpp
    /* 
        Will send a signed message over the specified endpoint and id 
    */
    void static sendSignedMessage(std::string data, EVP_PKEY * private_key, websocket_endpoint* endpoint, int id, int counter);
    /* 
        Iterates over a signed message and attempts to decrypt the aes key with it's own private key
        Returns the decrypted message.
        Function to decrypt a signed messsage's content, does not verify the reciever or check counter
        Ensure the entire message is provided.
    */
    std::string static decryptSignedMessage(std::string data, EVP_PKEY * private_key);

```

### Generating a chat message 

  Generate a chat message

  ```cpp
    /*Function to take a user message and a vector array of chat participants and convert it to a json string for encrypting in DataMessage*/
    std::string ChatMessage::generateChatMessage(std::string plaintext_message, std::vector<std::string> chat_participants);
  ```


### Generating a signed_data message
  
  Generate a signed_data messaage

  ```cpp
    /* 
        Used for creating the data in a chat message, currently missing client-info and time-to-die
        Returns the resultant string to be provided to the websocket or to be signed in signed_data function.
    */
    static std::string DataMessage::generateDataMessage(std::string text, std::vector<EVP_PKEY*> public_keys, std::vector<std::string> server_addresses);

    
  ``