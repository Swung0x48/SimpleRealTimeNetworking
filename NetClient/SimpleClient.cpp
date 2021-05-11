#include <iostream>
#include <blcl_net.h>

struct Vector {
    float x; float y; float z;
};
struct Quad {
    float x; float y; float z; float w;
};

struct State {
    uint32_t type;
    Vector pos;
    Quad rot;
};

enum class MsgType: uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    BallState,
    UsernameReq,
    Username,
    UsernameAck
};

class CustomClient: public blcl::net::client_interface<MsgType> {
private:
    std::string username_ = "Swung";
public:
    void send_username() {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::Username;
        uint8_t username[USERNAME_MAX_LENGTH_WITH_NULL];
        strcpy(reinterpret_cast<char *>(username), username_.c_str());
        msg << username;
        send(msg);
    }

    void ping_server() {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::ServerPing;

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
//        std::cout << sizeof(now) << "\n";
        msg << now;
        send(msg);
    }

    void broadcast_message() {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::BallState;
        State state;
        state.type = 0;
        state.pos = { 40, 15, -153 };
        state.rot = { 0, 0, 0, 0 };
        msg << state;
        send(msg);
    }
};

int main() {
    CustomClient c;
    c.connect("127.0.0.1", 60000);

//    std::cout << "[INFO] Sending a ping..." << "\n";
//    c.ping_server();
//    std::cout << "[INFO] Sending a ping..." << "\n";
//    c.ping_server();

    bool will_quit = false;
    auto thread = std::thread([&]() {
        while (!will_quit) {

            if (c.is_connected()) {
                if (!c.get_incoming_messages().empty()) {
                    auto msg = c.get_incoming_messages().pop_front().msg;

                    switch (msg.header.id) {
                        case MsgType::ServerAccept: {
                            std::cout << "[INFO] Server has accepted a connection.\n";
                            break;
                        }
                        case MsgType::ServerPing: {
                            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                            std::chrono::system_clock::time_point sent;
                            msg >> sent;
                            std::cout << "[INFO] Ping: " << std::chrono::duration<double, std::milli>(now - sent).count()
                                      << " ms\n";
                            break;
                        }
                        case MsgType::BallState: {
                            uint32_t client_id;
                            msg >> client_id;
                            std::cout << "Client ID: " << client_id << " Size: " << msg.size() << std::endl;

                            Vector pos;
                            Quad rot;
                            int ball = 0;
                            msg >> rot >> pos >> ball;
                            std::cout << "[INFO] Client " << client_id << ": " <<
                            "Ball: " << ball << " "
                            "(" << pos.x << ", " << pos.y << ", " << pos.z << "), " <<
                            "(" << rot.x << ", " << rot.y << ", " << rot.z << ", " << rot.w << ")" << "\n";
                            break;
                        }
                        case MsgType::UsernameReq: {
                            c.send_username();
                            break;
                        }
                        case MsgType::UsernameAck: {
                            uint8_t username[USERNAME_MAX_LENGTH_WITH_NULL];
                            assert(msg.size() <= USERNAME_MAX_LENGTH_WITH_NULL && msg.size() > 0);
                            std::memcpy(username, msg.body.data(), msg.size());
                            std::cout << username << std::endl;
                            break;
                        }
                    }
                }
            } else {
                std::cout << "[WARN] Server is going down..." << "\n";
                will_quit = true;
            }
        }
    });

    while (!will_quit) {
        int a; std::cin >> a;
        if (a == 1)
            c.ping_server();
        if (a == 2)
            c.broadcast_message();
    }
    if (thread.joinable())
        thread.join();
}