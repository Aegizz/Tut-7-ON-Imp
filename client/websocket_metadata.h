#ifndef WEBSOCKET_CONNECTION_METADATA_H
#define WEBSOCKET_CONNECTION_METADATA_H

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