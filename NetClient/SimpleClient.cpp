#include <iostream>
#include <blcl_net.h>

enum class MsgType: uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};

class CustomClient: public blcl::net::client_interface<MsgType> {
public:
    void ping_server() {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::ServerPing;

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::cout << sizeof(now) << "\n";
        msg << now;
        send(msg);
    }
};

int main() {
    CustomClient c;
    c.connect("127.0.0.1", 60000);

    std::thread([&]() {
        bool will_quit = false;
        while (!will_quit) {
            if (c.is_connected()) {
                if (!c.get_incoming_messages().empty()) {
                    auto msg = c.get_incoming_messages().pop_front().msg;

                    switch (msg.header.id) {
                        case MsgType::ServerPing:
                            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                            std::chrono::system_clock::time_point sent;
                            msg >> sent;
                            std::chrono::duration<double> elapsed = now - sent;
                            std::cout << "Ping: " << elapsed.count() << "\n";
                            break;
                    }
                }
            } else {
                std::cout << "[WARN] Server is going down...\n";
                will_quit = true;
            }
        }
    }).detach();

    while (true) {
        char a; std::cin >> a;
        std::cout << a;
        c.ping_server();
    }
}