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

class websocket_endpoint {
public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();
        
        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != "Open") {
                // Only close open connections
                continue;
            }
            
            std::cout << "> Closing connection " << it->second->get_id() << std::endl;
            
            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                          << ec.message() << std::endl;
            }
        }
        
        m_thread->join();
    }

    int connect(std::string const & uri) {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri));
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_close_handler(websocketpp::lib::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_message_handler(websocketpp::lib::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2
        ));

        m_endpoint.connect(con);

        return new_id;
    }
    void send(int id, const std::string& message, websocketpp::frame::opcode::value opcode, websocketpp::lib::error_code& ec) {
        // Get connection handle from connection id
        connection_metadata::ptr metadata = get_metadata(id);
        
        if (metadata) {
            m_endpoint.send(metadata->get_hdl(), message, opcode, ec);
        } else {
            ec = websocketpp::lib::error_code(websocketpp::error::invalid_state);  // Set error code if connection id is invalid
        }
    }

    void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        } else {
            return metadata_it->second;
        }
    }
private:
    typedef std::map<int,connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

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

void send_hello_message(websocket_endpoint* endpoint, int id){
    nlohmann::json data;

    // Format hello message
    data["type"] = "hello";
    data["public_key"] = "<Exported RSA public key>";
    data["client-info"] = "<client-id>-<server-id>";
    data["time-to-die"] = get_ttd();

    // Serialize JSON object
    std::string json_string = data.dump();

    // Send the message via the connection
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
    websocketpp::lib::error_code ec;
    endpoint->send(id, json_string, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cout << "> Error sending client list request message: " << ec.message() << std::endl;
    } else {
        std::cout << "> Client list request sent" << std::endl;
    }
}


int main() {
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

                if (close_code < 1000 && close_code > 4999) {
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
                    
                    if(msgType == "private"){
                        // Send private message
                    }else if(msgType == "public"){
                        // Send public message
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