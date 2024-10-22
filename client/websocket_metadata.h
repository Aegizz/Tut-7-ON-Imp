#ifndef WEBSOCKET_METADATA_H
#define WEBSOCKET_METADATA_H

#include <vector>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp> // For JSON library
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
//Self made client list implementation
#include "client_list.h"
//Self mage AES GCM Encryption with OpenSSL
#include "aes_encrypt.h"
#include "client.h"
#include "hexToBytes.h"
#include "client_signature.h"
#include "client_key_gen.h"
#include "signed_data.h"
// using to generate current time
#include <chrono>
#include <ctime>

class websocket_endpoint;

class connection_metadata {
private:
    ClientList * global_client_list;
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri, ClientList * client_list_pointer)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {
        global_client_list = client_list_pointer;
    }

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

    std::time_t current_time(){
        // Generate current time
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t convTime = std::chrono::system_clock::to_time_t(now);

        // Convert to GMT time and time structure
        std::tm* utc_tm = std::gmtime(&convTime);

        auto timepoint = std::mktime(utc_tm);

        return timepoint;
    }

    // Map to store latest counter for each user
    std::unordered_map <std::string, int> latestCounters;

    void on_message(client* c, websocketpp::connection_hdl hdl, client::message_ptr msg, std::string fingerprint, EVP_PKEY* privateKey) {
        // Vulnerable code: the payload without validation
        std::string payload = msg->get_payload();

        // Deserialize JSON message
        nlohmann::json messageJSON;
        try {
            // Attempt to parse the string as JSON
            messageJSON = nlohmann::json::parse(payload);
            
        }catch (nlohmann::json::parse_error& e) {
            // Catch parse error exception and display error message
            std::cerr << "Invalid JSON format: " << e.what() << std::endl;
        }
        //nlohmann::json messageJSON = nlohmann::json::parse(payload);
        std::string dataString;
        nlohmann::json data;

        if(messageJSON.contains("data")){
            dataString = messageJSON["data"].get<std::string>();
            try {
                // Attempt to parse the string as JSON
                data = nlohmann::json::parse(dataString);
                
            }catch (nlohmann::json::parse_error& e) {
                // Catch parse error exception and display error message
                std::cerr << "JSON format: " << e.what() << std::endl;
            }
        }
        
        if(messageJSON.contains("type")){
            if(messageJSON["type"] == "client_list"){
                std::cout << "\nClient list received" << std::endl;

                global_client_list->update(messageJSON);
                std::cout << "\n";

                std::pair<int, std::pair<int, std::string>> myInfo = global_client_list->retrieveClientFromFingerprint(fingerprint);
                if(myInfo.first != -1){
                    global_client_list->printUsers(myInfo.first, myInfo.second.first);
                }
            }else if(data["type"] == "public_chat"){
                if(data.contains("sender") && messageJSON.contains("signature") && messageJSON.contains("counter") && data.contains("message")){

                }else{
                    std::cerr << "Invalid JSON provided" << std::endl;
                    return;
                }
                std::pair<int, std::pair<int, std::string>> chatInfo = global_client_list->retrieveClientFromFingerprint(data["sender"]);

                if(chatInfo.first == -1){
                    std::cout << "Invalid fingerprint received in public message" << std::endl;
                    return;
                }

                int server_id = chatInfo.first;
                int client_id = chatInfo.second.first;
                std::string public_key = chatInfo.second.second;
                EVP_PKEY* pubKey = Client_Key_Gen::stringToPEM(public_key);

                std::string signature = messageJSON["signature"];
                int counter = messageJSON["counter"];

                if(!ClientSignature::verifySignature(signature, data.dump() + std::to_string(counter), pubKey)){
                    std::cout << "Invalid signature" << std::endl;
                    return;
                }
                std::cout << "Verified signature" << std::endl;

                // counter check
                if (counter <= latestCounters[signature]) {
                    std::cout << "Replay attack detected! Message discarded." << std::endl;
                    return;
                }        

                //update latest counter
                latestCounters[signature] = counter;

                std::cout << "Public chat received from client " << client_id << " on server " << server_id << std::endl;
                
                std::cout << data["message"] << std::endl;
            }else if(data["type"] == "chat"){
                std::string decrypted_str = SignedData::decryptSignedMessage(messageJSON.dump(), privateKey);
                if(decrypted_str == ""){
                    return;
                }

                try {
                    // Attempt to parse the string as JSON
                    nlohmann::json chat = nlohmann::json::parse(decrypted_str);
                    if(chat.contains("participants") && messageJSON.contains("signature") && messageJSON.contains("counter") && chat.contains("message") && data.contains("time-to-die")){

                    }else{
                        std::cerr << "Invalid JSON provided" << std::endl;
                        return;
                    }
                    
                    std::string ttd_timestamp_str = data["time-to-die"];

                    //parse the TTD timestamp
                    std::tm ttd_tm = {};
                    std::istringstream ss(ttd_timestamp_str);
                    ss >> std::get_time(&ttd_tm, "%Y-%m-%dT%H:%M:%SZ");

                    if (ss.fail()) {
                        std::cout << "Invalid TTD Format";
                    }

                    std::time_t ttd_timepoint = std::mktime(&ttd_tm);

                    std::time_t now = current_time();

                    if (now >= ttd_timepoint) {
                        std::cout << "Message expired based on TTD, discarding packet." << std::endl;
                        return;
                    }


                    // Convert participants to a std::vector<std::string>
                    std::vector<std::string> participants = chat["participants"].get<std::vector<std::string>>();

                    // If parsing is successful, you can work with the JSON object
                    std::pair<int, std::pair<int, std::string>> chatInfo = global_client_list->retrieveClientFromFingerprint(participants[0]);
                    if(chatInfo.first == -1){
                        std::cout<<"Invalid fingerprint received in message"<<std::endl;
                        return;
                    }

                    int server_id = chatInfo.first;
                    int client_id = chatInfo.second.first;
                    std::string public_key = chatInfo.second.second;
                    EVP_PKEY* pubKey = Client_Key_Gen::stringToPEM(public_key);

                    std::string signature = messageJSON["signature"];
                    int counter = messageJSON["counter"];

                    if(!ClientSignature::verifySignature(signature, data.dump() + std::to_string(counter), pubKey)){
                        std::cout << "Invalid signature" << std::endl;
                        return;
                    }
                    std::cout << "Verified signature" << std::endl;

                    // counter check
                    if (counter <= latestCounters[signature]) {
                        std::cout << "Replay attack detected! Message discarded." << std::endl;
                        return;
                    }        

                    //update latest counter
                    latestCounters[signature] = counter;

                    std::cout << "Chat received from client " << client_id << " on server " << server_id << std::endl << std::endl;
                    std::cout << chat["message"] << "\n" << std::endl;
                    std::cout << "All recipients:" << std::endl;
                    for(int i=1; i<(int)participants.size(); i++){
                        std::pair<int, std::pair<int, std::string>> chatInfo = global_client_list->retrieveClientFromFingerprint(participants[i]);
                        if(chatInfo.first == -1){
                            std::cout<<"Invalid recipient fingerprint received in message"<<std::endl;
                            continue;
                        }
                        server_id = chatInfo.first;
                        client_id = chatInfo.second.first;
                        std::cout << "ServerID: " << server_id << " ClientID: " << client_id << std::endl;
                    }
                }catch (nlohmann::json::parse_error& e) {
                    // Catch parse error exception and display error message
                    std::cerr << "Decrypted message is an Invalid JSON format: " << e.what() << std::endl;
                }
                
            }else{
                // Print the received message
                std::cout << "> Invalid message type received: " << payload << std::endl;
            }
        }else{
            std::cerr << "Invalid JSON provided" << std::endl;
            return;
        }
        std::cout << "\n";
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

#endif