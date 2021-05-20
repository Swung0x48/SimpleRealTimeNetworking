#include <iostream>
#include <blcl_net.h>

struct Vector {
    float x = 0; float y = 0; float z = 0;
};
struct Quad {
    float x = 0; float y = 0; float z = 0; float w = 0;
};

struct State {
    uint32_t type = 0;
    Vector pos;
    Quad rot;
};

enum class MsgType: uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    ClientDisconnect,
    BallState,
    UsernameReq,
    Username,
    UsernameAck,
    EnterMap,
    FinishLevel,
    ExitMap,
    MapHashReq,
    MapHash,
    MapHashAck
};


class CustomClient: public blcl::net::client_interface<MsgType> {
private:
    std::string username_ = "Swung";
public:
    uint32_t max_username_length_ = 0;
    void send_username() {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::Username;
        uint8_t username_bin_[max_username_length_ + 1];
        strncpy(reinterpret_cast<char *>(username_bin_), username_.c_str(), max_username_length_);
        msg.write(username_bin_, strlen(reinterpret_cast<char *>(username_bin_)) + 1);
        send(msg);
    }

    void ping_server() {
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::ServerPing;

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
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
                            uint64_t client_id;
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
                            msg >> c.max_username_length_;
                            c.send_username();
                            break;
                        }
                        case MsgType::UsernameAck: {
                            std::cout << reinterpret_cast<const char*>(msg.body.data()) << std::endl;
                            break;
                        }
                        case MsgType::ClientDisconnect: {
                            uint64_t client_id; msg >> client_id;
                            char username_bin_[c.max_username_length_ + 1];
                            strcpy(username_bin_, reinterpret_cast<const char *>(msg.body.data()));
                            std::cout << "[INFO] " << '[' << client_id << "] " << username_bin_ << " left the game." << std::endl;
                            break;
                        }
                        default: {
                            std::cerr << "[ERR]: Unknown Message ID: " << (uint32_t) msg.header.id << std::endl;
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