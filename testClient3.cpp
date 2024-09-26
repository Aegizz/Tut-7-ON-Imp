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

// Hard coded public key for this client instance
//const std::string PUBLIC_KEY = "ABCDEF";
const std::string PUBLIC_KEY = "GHIJKL";
//const std::string PUBLIC_KEY = "MNOPQR";

//Global pointer for client list
ClientList * global_client_list = nullptr;


typedef websocketpp::client<websocketpp::config::asio_client> client;

class connection_metadata {
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {}

    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Open";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }
    
    void on_close(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Closed";
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " (" 
          << websocketpp::close::status::get_string(con->get_remote_close_code()) 
          << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
        std::cout << s.str() << std::endl;
    }

    void on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg) {
        // Vulnerable code: the payload without validation
        std::string payload = msg->get_payload();

        // Deserialize JSON message
        nlohmann::json data = nlohmann::json::parse(payload);

        if(data["type"] == "client_list"){
            std::cout << "Client list received: " << payload << std::endl;
            if (global_client_list != nullptr){
                delete global_client_list;
            }
            // Process client list
            global_client_list = new ClientList(data);
        }else{
            // Print the received message
            std::cout << "> Message received: " << payload << std::endl;
        }
    }

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }
    
    int get_id() const {
        return m_id;
    }
    
    std::string get_status() const {
        return m_status;
    }

    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
};

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
        
        /*for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
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
        }*/
        
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
        }else if (metadata_it->second->get_status() != "Open") {
            // Only close open connections
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
        std::cout << "> Client list request sent" << std::endl;
    }
}



int main() {
    bool done = false;
    std::string input;
    websocket_endpoint endpoint;

    int initId = endpoint.connect("ws://localhost:9002");
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
                    int id = endpoint.connect(input.substr(8));
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