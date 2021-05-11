#include <iostream>
#include <blcl_net.h>

enum class MsgType: uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    BallState,
    UsernameReq,
    Username,
    UsernameAck
};

struct ClientData {
    std::string username;
};

class CustomServer: public blcl::net::server_interface<MsgType> {
private:
    std::unordered_map<uint64_t, ClientData> users_; // Possible race condition writing to this.
public:
    CustomServer(uint16_t port) : blcl::net::server_interface<MsgType>(port) {

    }

protected:
    bool on_client_connect(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::ServerAccept;
        client->send(msg);

        return true;
    }

    void on_client_disconnect(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        std::cout << "[INFO] Client " << client->get_id() << " has been disconnected." << std::endl;
    }

    void on_client_validated(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::UsernameReq;
        client->send(msg);
    }

    void on_message(std::shared_ptr<blcl::net::connection<MsgType>> client, blcl::net::message<MsgType>& msg) override {
        switch (msg.header.id) {
            case MsgType::ServerPing: {
                client->send(msg);
                break;
            }
            case MsgType::BallState: {
                msg << client->get_id();
                broadcast_message(msg, get_online_clients(), client);
                break;
            }
            case MsgType::Username: {
                uint8_t username[USERNAME_MAX_LENGTH_WITH_NULL];
                assert(msg.size() <= USERNAME_MAX_LENGTH_WITH_NULL && msg.size() > 0);
                std::memcpy(username, msg.body.data(), msg.size());
                ClientData data;
                data.username = std::string(reinterpret_cast<char*>(username));
                users_[client->get_id()] = data;
                std::cout << data.username << " joined the server." << std::endl;

                msg.header.id = MsgType::UsernameAck;
                client->send(msg);
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
