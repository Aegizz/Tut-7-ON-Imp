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

// **NOTE:** This file is a snapshot of the WebSocket++ utility client tutorial.
// Additional related material can be found in the tutorials/utility_client
// directory of the WebSocket++ repository.

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
#include <sstream>
#include <nlohmann/json.hpp> // For JSON library

// For UTC timestamp
#include <chrono>
#include <ctime> 


//Self made client list implementation
#include "client/client_list.h"

//Self made AES GCM Encryption with OpenSSL
#include "client/aes_encrypt.h"

// Hard coded public key for this client instance
//const std::string PUBLIC_KEY = "ABCDEF";
const std::string PUBLIC_KEY = "GHIJKL";
//const std::string PUBLIC_KEY = "MNOPQR";

//Global pointer for client list
ClientList * global_client_list = nullptr;


void send_message(client& c, websocketpp::connection_hdl hdl, const std::string& message) {
    websocketpp::lib::error_code ec;
    
    // Send the message as a text frame
    c.send(hdl, message, websocketpp::frame::opcode::text, ec);
    
    if (ec) {
        std::cout << "Error sending message: " << ec.message() << std::endl;
    }
}

std::string get_ttd(){
    // Generate current time
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    std::chrono::minutes minute(1);

    // Add one minute
    std::chrono::system_clock::time_point newTime = now + minute;

    std::time_t convTime = std::chrono::system_clock::to_time_t(newTime);

    // Convert to GMT time and time structure
    std::tm* utc_tm = std::gmtime(&convTime);

    // Convert to ISO 8601 structure
    std::stringstream timeString;
    timeString << std::put_time(utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    
    return timeString.str();
}

bool is_connection_open(websocket_endpoint* endpoint, int id){
    connection_metadata::ptr metadata = endpoint->get_metadata(id);

    // Do not continue until websocket has finished connecting
    if(metadata->get_status() == "Open"){
        return true;
    }
    return false;
}

void send_hello_message(websocket_endpoint* endpoint, int id){
    nlohmann::json user;

    // Format hello message
    user["type"] = "hello";
    user["public_key"] = PUBLIC_KEY;
    user["time-to-die"] = get_ttd();

    nlohmann::json data;
    data["data"] = user;

    // Serialize JSON object
    std::string json_string = data.dump();

    // Send the message via the connection
    if(!is_connection_open(endpoint, id)){
        return;
    }
    websocketpp::lib::error_code ec;
    endpoint->send(id, json_string, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cout << "> Error sending hello message: " << ec.message() << std::endl;
    } else {
        std::cout << "> Hello message sent" << std::endl;
    }
}

void send_client_list_request(websocket_endpoint* endpoint, int id){
    nlohmann::json data;

    // Format client list request message
    data["type"] = "client_list_request";

    // Serialize JSON object
    std::string json_string = data.dump();

    // Send the message via the connection
    if(!is_connection_open(endpoint, id)){
        return;
    }
    websocketpp::lib::error_code ec;
    endpoint->send(id, json_string, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cout << "> Error sending client list request message: " << ec.message() << std::endl;
    } else {
        std::cout << "> Client list request sent" << "\n" << std::endl;
    }
}



int main() {
    bool done = false;
    std::string input;
    websocket_endpoint endpoint;

    int initId = endpoint.connect("ws://localhost:9004", global_client_list);
    //int initId = endpoint.connect("ws://172.30.30.134:9002");
    if (initId != -1) {
        std::cout << "> Created connection with id " << initId << std::endl;

        connection_metadata::ptr metadata = endpoint.get_metadata(initId);

        // Do not continue until websocket has finished connecting
        while(metadata->get_status() == "Connecting"){
            metadata = endpoint.get_metadata(initId);
        }

        // Send server intialization messages
        send_hello_message(&endpoint, initId);

        send_client_list_request(&endpoint, initId);
        
        sleep(60);
        
        int close_code = websocketpp::close::status::normal;
        endpoint.close(initId, close_code, "Reached end of run");
    }
    return 0;



// Time wasted fixing: 2hrs
// Main loop in case of need for testing DO NOT USE UNLESS NEEDED.
// Bash script will constantly ask for input causing the log file to just be filled with "Enter Command: " literally took a gigabyte of storage on my computer.

    while (!done) {
        std::cout << "Enter Command: ";
        if (std::getline(std::cin, input)){
            if (input == "quit") {
                done = true;
            } else if (input == "help") {
                std::cout
                    << "\nCommand List:\n"
                    << "connect <ws uri>\n"
                    << "send <connection id>\n"
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

                        // Send server intialization messages
                        send_hello_message(&endpoint, id);

                        send_client_list_request(&endpoint, id);
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
                    std::cout << "> Incorrect usage of command 'send <connection id>'" << std::endl;
                }else{
                    std::stringstream ss(input);
                    std::string cmd;
                    int id;

                    // Extract command and id from input
                    ss >> cmd >> id;

                    // Get metadata of the connection
                    connection_metadata::ptr metadata = endpoint.get_metadata(id);

                    if (metadata) {
                        std::cout << "Enter message to send: ";
                        std::string message;
                        std::getline(std::cin, message);  // Get the message from the user

                        nlohmann::json data;
                        data["type"] = "chat";
                        data["destination_servers"] = nlohmann::json::array({ "<Address of each recipient's destination server>" });
                        data["iv"] = "<Base64 encoded AES initialization vector>";
                        data["symm_keys"] = nlohmann::json::array({ "<Base64 encoded AES key, encrypted with each recipient's public RSA key>" });
                        data["chat"] = message;
                        data["client-info"] = "<client-id>-<server-id>";
                        data["time-to-die"] = get_ttd();

                        // Serialize JSON object
                        std::string json_string = data.dump();

                        // Send the message via the connection
                        websocketpp::lib::error_code ec;
                        endpoint.send(id, json_string, websocketpp::frame::opcode::text, ec);

                        if (ec) {
                            std::cout << "> Error sending message: " << ec.message() << std::endl;
                        } else {
                            std::cout << "> Message sent" << std::endl;
                        }

                    } else {
                        std::cout << "> Unknown connection id " << id << std::endl;
                    }
                }
            } else {
                std::cout << "> Unrecognized Command" << std::endl;
            }
        }
    }

    return 0;
}