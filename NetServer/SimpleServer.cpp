#include <iostream>
#include <blcl_net.h>

enum class MsgType: uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomServer: public blcl::net::server_interface<MsgType> {
public:
    CustomServer(uint16_t port) : blcl::net::server_interface<MsgType>(port) {

    }

protected:
    bool on_client_connect(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        return true;
    }

    void on_client_disconnect(std::shared_ptr<blcl::net::connection<MsgType>> client) override {

    }

    void on_message(std::shared_ptr<blcl::net::connection<MsgType>> client, blcl::net::message<MsgType>& msg) override {
        switch (msg.header.id) {
            case MsgType::ServerPing:
                std::cout << "[INFO] " << client->get_id() << ": Server Ping" << std::endl;
                client->send(msg);
                break;
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