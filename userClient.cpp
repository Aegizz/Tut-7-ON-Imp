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
const int ClientNumber = 1;

// Create key file names
std::string privFileName = "client/private_key" + std::to_string(ClientNumber) + ".pem";
std::string pubFileName = "client/public_key" + std::to_string(ClientNumber) + ".pem";

// Define keys
EVP_PKEY* privKey;
EVP_PKEY* pubKey;

//Global pointer for client list
ClientList * global_client_list = nullptr;

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

    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit") {
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "connect <ws uri>\n"
                << "send <message type> <connection id>\n"
                << "close <connection id> [<close code:default=1000>] [<close reason>]\n"
                << "show <connection id>\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        } else if (input.substr(0,7) == "connect") {
            if((int)input.size() == 7){
                std::cout << "> Incorrect usage of command 'connect <ws uri>'" << std::endl;
            }else{
                int id = endpoint.connect(input.substr(8), global_client_list);
                if (id != -1) {
                    std::cout << "> Created connection with id " << id << std::endl;

                    connection_metadata::ptr metadata = endpoint.get_metadata(id);

                    // Do not continue until websocket has finished connecting
                    while(metadata->get_status() == "Connecting"){
                        metadata = endpoint.get_metadata(id);
                    }

                    // Send hello message
                    ClientUtilities::send_hello_message(&endpoint, id, privKey, pubKey, 12345);

                    // Send client list request
                    ClientUtilities::send_client_list_request(&endpoint, id);
                }
            }
        } else if (input.substr(0,5) == "close") {
            if((int)input.size() == 5){
                std::cout << "> Incorrect usage of command 'close <connection id> [<close code:default=1000>] [<close reason>]'" << std::endl;
            }else{
                std::stringstream ss(input);
                
                std::string cmd;
                int id;
                int close_code = websocketpp::close::status::normal;
                std::string reason;
                
                ss >> cmd >> id >> close_code;

                if (id < 0) {
                    std::cout << "> Invalid connenction id: " << id << std::endl;
                    continue;
                }

                if (close_code < 1000 && close_code > 3999) {
                    std::cout << "> Invalid close code: " << close_code << std::endl;
                    continue;
                }

                std::getline(ss,reason);
                endpoint.close(id, close_code, reason);
            }
        }  else if (input.substr(0,4) == "show") {
            if((int)input.size() == 4){
                std::cout << "> Incorrect usage of command 'show <connection id>'" << std::endl;
            }else{
                int id = atoi(input.substr(5).c_str());

                connection_metadata::ptr metadata = endpoint.get_metadata(id);
                if (metadata) {
                    std::cout << *metadata << std::endl;
                } else {
                    std::cout << "> Unknown connection id " << id << std::endl;
                }
            }
        } else if (input.substr(0,4) == "send") {
            if((int)input.size() == 4){
                std::cout << "> Incorrect usage of command 'send <message type> <connection id>'" << std::endl;
            }else{
                std::stringstream ss(input);
                std::string cmd;
                std::string msgType;
                int id;

                // Extract command and id from input
                ss >> cmd >> msgType >> id;

                // Get metadata of the connection
                connection_metadata::ptr metadata = endpoint.get_metadata(id);

                if (metadata) {
                    std::cout << "Enter message to send: ";
                    std::string message;
                    std::getline(std::cin, message);  // Get the message from the user
                    
                    if (msgType == "private") { 
                        // Send private message
                        bool done2 = false;
                        std::vector<int> server_ids;
                        std::vector<int> client_ids;
                        while (!done2) {
                            // Get Server ID
                            std::cout << "Which Server ID is the Client: " << std::endl;
                            std::string server;
                            std::getline(std::cin, server);
                            try {
                                int server2 = stoi(server);
                                server_ids.push_back(server2);
                            } catch (const std::invalid_argument& e) {
                                std::cout << "Invalid argument: Cannot convert '"<< server << "'to an integer." << std::endl;
                                continue;
                            }
                            
                            //Get Client ID
                            std::cout << "Please Provide the Client ID: " << std::endl;
                            std::string client;
                            std::getline(std::cin, client);
                            try {
                                int client2 = stoi(client);
                                client_ids.push_back(client2);
                            } catch (const std::invalid_argument& e) {
                                std::cout << "Invalid argument: Cannot convert '"<< client << "'to an interger." << std::endl;
                                server_ids.pop_back();
                                continue;
                            }

                            // check wheter user has finished selecting their clients
                            bool done3 = false;
                            while(!done3) {
                                std::cout << "Are finished adding all the clients you to message too? (YES/NO): " << std::endl;
                                std::string yes_or_no;
                                std::getline(std::cin, yes_or_no);
                                if (yes_or_no == "YES") {
                                    done3 = true;
                                    done2 = true;
                                    std::vector<EVP_PKEY*> list_public_keys;
                                    
                                    for (int i = 0; i < server_ids.size(); i++) {
                                    std::string public_key = global_client_list->retrieveClient(server_ids[i], client_ids[i]).second;
                                    list_public_keys.push_back(Client_Key_Gen::stringToPEM(public_key));
                                    }
                                    
                                    ClientUtilities::send_chat(&endpoint, id, message, privKey, pubKey,list_public_keys,,12345,);  //send to server
                                } else if (yes_or_no == "NO") {
                                    std::cout << "Ok, let's add more clients" << std::endl;
                                    done3 = true;
                                } else {
                                    std::cout << "Input Invalid: Must be either YES or NO." << std::endl;
                                }

                            }
                        }
    
                    } else if(msgType == "public"){
                        // Send public message
                        ClientUtilities::send_public_chat(&endpoint, id, message, privKey, pubKey, 12345);  //send to server
                    }   
                } else {
                    std::cout << "> Unknown connection id " << id << std::endl;
                }
            }
        } else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}
