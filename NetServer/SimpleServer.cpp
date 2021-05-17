#include <iostream>
#include <blcl_net.h>
#include <unordered_map>

enum class MsgType: uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomServer: public blcl::net::server_interface<MsgType> {
private:
    std::unordered_map<std::string, uint64_t> fail2ban_counter_;
    uint64_t max_fail_attempt = 10;
    bool is_banned(const std::shared_ptr<blcl::net::connection<MsgType>>& client) {
        if (fail2ban_counter_[client->get_endpoint().address().to_string()] > max_fail_attempt)
            return true;

        ++fail2ban_counter_[client->get_endpoint().address().to_string()];
        return false;
    }

public:
    CustomServer(uint16_t port) : blcl::net::server_interface<MsgType>(port) {

    }

protected:
    bool on_client_connect(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        if (is_banned(client)) {
            return false;
        }

        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::ServerAccept;
        client->send(msg);

        return true;
    }

    void on_client_disconnect(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        std::cout << "[INFO] Client " << client->get_id() << " has been disconnected." << std::endl;
    }

    void on_message(std::shared_ptr<blcl::net::connection<MsgType>> client, blcl::net::message<MsgType>& msg) override {
        switch (msg.header.id) {
            case MsgType::ServerPing: {
                //std::cout << "[INFO] " << client->get_id() << ": Server Ping" << std::endl;
                client->send(msg);
                break;
            }
            case MsgType::MessageAll: {
               // std::cout << "[INFO] [" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << client->get_id() << ": Broadcast\n";
//                blcl::net::message<MsgType> msg;
                msg.header.id = MsgType::ServerMessage;
                msg << client->get_id();
                broadcast_message(msg, client);
            }
        }
    }
};

int main() {
    CustomServer server(60000);
    server.start();

    while (1) {
        server.update();
    }

    return 0;
}
