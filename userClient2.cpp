/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// **NOTE:** This file is based on the WebSocket++ utility client tutorial.

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "client/websocket_endpoint.h"
#include "client/websocket_metadata.h"

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp> // For JSON library


#include "client/client_list.h" // Self made client list implementation
#include "client/aes_encrypt.h" // AES GCM Encryption with OpenSSL
#include "client/client_key_gen.h" // OpenSSL Key generation
#include "client/client_utilities.h" // For sending messages, checking connections
#include "client/Fingerprint.h" // For fingerprint generation

// Used to differentiate client processses locally
const int ClientNumber = 2;

// Create key file names
std::string privFileName = "client/private_key" + std::to_string(ClientNumber) + ".pem";
std::string pubFileName = "client/public_key" + std::to_string(ClientNumber) + ".pem";

// Define keys
EVP_PKEY* privKey;
EVP_PKEY* pubKey;

//Global pointer for client list
ClientList* global_client_list = new ClientList;

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason);

    return out;
}

int main() {
    // Load keys
    privKey = Client_Key_Gen::loadPrivateKey(privFileName.c_str());
    pubKey = Client_Key_Gen::loadPublicKey(pubFileName.c_str());

    // If keys files don't exist, create keys and load from newly created files
    if(!privKey || !pubKey){
        std::cout << "\nCreating key files\n" << std::endl;
        if(!Client_Key_Gen::key_gen(ClientNumber)){
            privKey = Client_Key_Gen::loadPrivateKey(privFileName.c_str());
            pubKey = Client_Key_Gen::loadPublicKey(pubFileName.c_str());
        }else{
            std::cout << "Could not load keys" << std::endl;
            return 1;
        }
    }

    bool done = false;
    std::string input;
    std::string fingerprint = Fingerprint::generateFingerprint(pubKey);
    websocket_endpoint endpoint(fingerprint, privKey);
    int currentID=-1;

    while (!done) {
        std::pair<int, std::pair<int, std::string>> myInfo = global_client_list->retrieveClientFromFingerprint(fingerprint);
        if(myInfo.first != -1){
            global_client_list->printUsers(myInfo.first, myInfo.second.first);
        }

        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit") {
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "connect\n"
                << "send <message type>\n"
                << "close [<close code:default=1000>] [<close reason>]\n"
                << "show\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        } else if (input == "connect") {
            bool validUri=false;
            while(!validUri){
                std::cout << "Enter server uri: " << std::endl;
                std::string uri;
                std::getline(std::cin, uri);  // Get the message from the user
                
                if((int)uri.size() > 5){
                    if(uri.substr(0,5) == "ws://"){
                        validUri=true;
                        currentID = endpoint.connect(uri, global_client_list);
                        if (currentID != -1) {
                            std::cout << "> Initiated connection to " << uri << std::endl;

                            connection_metadata::ptr metadata = endpoint.get_metadata(currentID);

                            // Do not continue until websocket has finished connecting
                            while(metadata->get_status() == "Connecting"){
                                metadata = endpoint.get_metadata(currentID);
                            }
                            if(metadata->get_status() == "Failed"){
                                std::cout << "Connection Failed to " << uri << std::endl;
                                continue;
                            }else if(metadata->get_status() == "Open"){
                                std::cout << "> Established connection with " << uri << std::endl;
                                // Send hello message
                                ClientUtilities::send_hello_message(&endpoint, currentID, privKey, pubKey, 12345);

                                // Send client list request
                                ClientUtilities::send_client_list_request(&endpoint, currentID);
                            }
                        }
                    }else{
                        std::cout << "Not a valid URI" << std::endl;
                        continue;
                    }
                }else{
                    std::cout << "Uri not long enough" << std::endl;
                    continue;
                }
            }
        } else if (input.substr(0,5) == "close") {
            std::stringstream ss(input);
            
            std::string cmd;
            int close_code = websocketpp::close::status::normal;
            std::string reason;
            
            ss >> cmd >> close_code;

            if (close_code < 1000 && close_code > 3999) {
                std::cout << "> Invalid close code: " << close_code << std::endl;
                continue;
            }

            std::getline(ss,reason);
            endpoint.close(currentID, close_code, reason);
        }  else if (input.substr(0,4) == "show") {
            connection_metadata::ptr metadata = endpoint.get_metadata(currentID);
            if (metadata) {
                std::cout << *metadata << std::endl;
            } else {
                std::cout << "> No connection established" << std::endl;
            }
        } else if (input.substr(0,4) == "send") {
            if((int)input.size() == 4){
                std::cout << "> Incorrect usage of command 'send <message type> <connection id>'" << std::endl;
            }else{
                std::stringstream ss(input);
                std::string cmd;
                std::string msgType;

                // Extract command and id from input
                ss >> cmd >> msgType;

                // Get metadata of the connection
                connection_metadata::ptr metadata = endpoint.get_metadata(currentID);

                if (metadata) {
                    std::cout << "Enter message to send: " << std::endl;
                    std::string message;
                    std::getline(std::cin, message);  // Get the message from the user

                    bool validType=false;

                    while(!validType){
                        // If a private message was specified
                        if (msgType == "private"){ 
                            validType=true;

                            bool clientsEntered = false;
                            std::vector<EVP_PKEY*> list_public_keys;
                            std::vector<std::string> destination_servers;
                            std::vector<std::string> public_keys_strings;

                            while (!clientsEntered) {
                                // Get Server ID
                                std::cout << "Enter the Client's Server ID. If done entering client's type \"done\" or type cancel if you don't want to continue sending a message." << std::endl;
                                std::string serverString="";
                                int serverInt=-1;
                                std::getline(std::cin, serverString);
                                if(serverString == "done"){
                                    if((int)list_public_keys.size() == 0){
                                        std::cout << "No clients provided. You need to specify clients." << std::endl;
                                        continue;
                                    }else{
                                        ClientUtilities::send_chat(&endpoint, currentID, message, privKey, pubKey, list_public_keys, destination_servers, 12345);  //send to server
                                        break;
                                    }
                                }else if(serverString == "cancel"){
                                    validType=true;
                                    clientsEntered=true;
                                    continue;
                                }

                                try {
                                    serverInt = std::stoi(serverString);

                                } catch (const std::invalid_argument& e) {
                                    std::cout << "Invalid argument: " << serverString << " is not a valid Server ID." << std::endl;
                                    continue;
                                }
                                
                                //Get Client ID
                                std::cout << "Enter the Client ID: If done entering client's type \"done\" or type cancel if you don't want to continue sending a message. " << std::endl;
                                std::string clientString="";
                                int clientInt=-1;
                                std::getline(std::cin, clientString);
                                if(clientString == "done"){
                                    if((int)list_public_keys.size() == 0){
                                        std::cout << "No clients provided. You need to specify clients." << std::endl;
                                        continue;
                                    }else{
                                        ClientUtilities::send_chat(&endpoint, currentID, message, privKey, pubKey, list_public_keys, destination_servers, 12345);  //send to server
                                        break;
                                    }
                                }else if(clientString == "cancel"){
                                    validType=true;
                                    clientsEntered=true;
                                    continue;
                                }
                                
                                try {
                                    clientInt = std::stoi(clientString);
                                } catch (const std::invalid_argument& e) {
                                    std::cout << "Invalid argument: " << clientString << " is not a valid Client ID." << std::endl;
                                    continue;
                                }
                                
                                std::pair<int, std::string> retClient = global_client_list->retrieveClient(serverInt, clientInt);
                                if(retClient.second == ""){
                                    continue;
                                }
                                std::string public_key = retClient.second;

                                if(std::find(public_keys_strings.begin(), public_keys_strings.end(), public_key) != public_keys_strings.end()){
                                    std::cout << "Client " << clientInt << " on Server " << serverInt << " is already a recipient" << std::endl;
                                    continue;
                                }
                                
                                std::string genFingerprint = Fingerprint::generateFingerprint(Client_Key_Gen::stringToPEM(public_key));
                                if(fingerprint == genFingerprint){
                                    std::cout << "You cannot be a recipient of your own message" << std::endl;
                                    continue;
                                }

                                public_keys_strings.push_back(public_key);
                                destination_servers.push_back(global_client_list->retrieveAddress(serverInt));
                                list_public_keys.push_back(Client_Key_Gen::stringToPEM(public_key));

                                // Check whether user has finished selecting their clients
                                bool validYesNo = false;
                                while(!validYesNo) {
                                    std::cout << "Are you finished adding all the clients you want to send the message to? (Yes/No): " << std::endl;
                                    std::string yes_or_no;
                                    std::getline(std::cin, yes_or_no);

                                    // Convert input to uppercase to handle case-insensitivity
                                    for (auto &c : yes_or_no) {
                                        c = std::toupper(c);
                                    }
                                    
                                    if (yes_or_no == "YES") {
                                        validYesNo = true;
                                        clientsEntered = true;

                                        ClientUtilities::send_chat(&endpoint, currentID, message, privKey, pubKey, list_public_keys, destination_servers, 12345);  //send to server
                                    } else if (yes_or_no == "NO") {
                                        std::cout << "Ok, let's add more clients" << std::endl;
                                        validYesNo = true;
                                    } else {
                                        std::cout << "Input Invalid: Must be either YES or NO." << std::endl;
                                        continue;
                                    }
                                }
                            }
        
                        } else if(msgType == "public"){
                            validType=true;

                            ClientUtilities::send_public_chat(&endpoint, currentID, message, privKey, pubKey, 12345);  //send to server
                        }else{
                            std::cout << "Invalid message type entered\nValid types are \"private\" and \"public\"" << std::endl;
                            break;
                        }   
                    }
                } else {
                    std::cout << "> No connection established" << std::endl;
                }
            }
        } else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}