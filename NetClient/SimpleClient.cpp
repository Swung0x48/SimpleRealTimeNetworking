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
    ClientValidated,
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
    MapHashAck,

    PlayerJoined
};


class CustomClient: public blcl::net::client_interface<MsgType> {
private:
    std::string username_ = "Spectator001";
    std::unordered_map<uint64_t, std::string> active_clients_;
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

    void add_active_client(uint64_t client, const std::string& player_name) {
        active_clients_[client] = player_name;
    }
};

int main() {
    CustomClient c;
    c.connect("139.224.23.40", 60000);

    bool will_quit = false;
    auto thread = std::thread([&]() {
        while (!will_quit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            if (c.is_connected()) {
                if (!c.get_incoming_messages().empty()) {
                    auto msg = c.get_incoming_messages().pop_front().msg;

                    switch (msg.header.id) {
                        case MsgType::ServerAccept: {
                            std::cout << "[INFO] Server has accepted a connection.\n";
                            break;
                        }
                        case MsgType::ClientValidated: {
                            uint64_t id; msg >> id;
                            std::cout << "[INFO] Assigned client ID: " << id << std::endl;
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
                        case MsgType::Username: {
                            uint64_t client_id; msg >> client_id;
                            std::string name(reinterpret_cast<const char *>(msg.body.data()));
                            c.add_active_client(client_id, name);
                            std::cout << "[INFO] " << client_id << " " << name << std::endl;
                            break;
                        }
                        case MsgType::UsernameAck: {
                            std::cout << reinterpret_cast<const char*>(msg.body.data()) << std::endl;
                            break;
                        }
                        case MsgType::MapHashReq: {
                            std::cout << "[INFO] Map hash required!" << std::endl;
                            break;
                        }
                        case MsgType::MapHashAck: {
                            std::cout << "[INFO] Map hash accepted." << std::endl;
                            break;
                        }
                        case MsgType::ClientDisconnect: {
                            uint64_t client_id; msg >> client_id;
                            char username_bin_[c.max_username_length_ + 1];
                            strcpy(username_bin_, reinterpret_cast<const char *>(msg.body.data()));
                            std::cout << "[INFO] " << '[' << client_id << "] " << username_bin_ << " left the game." << std::endl;
                            break;
                        }
                        case MsgType::ExitMap: {
                            uint64_t client_id; msg >> client_id;
                            std::cout << "[INFO] " << "Client " << client_id << " has exited this map." << std::endl;
                            break;
                        }
                        case MsgType::PlayerJoined: {
                            std::cout << "[INFO] " << reinterpret_cast<const char *>(msg.body.data()) << " joined the game.";
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

    std::vector<std::string> map_hashes = {
            "88372daf54efd05dcab12010f19cb111dcb5827bf9fa0a4d8ef70806637a039a",
            "4594d20c0a9c54f2b1f049ae8151350f18918502eb57834dc7a2953ced517616",
            "cbb1225713f00449b32d1bbe28c36f6b0f5723f18e590d8aa3c5624a88e15a5b",
            "218baee114d871b76852e7a605aedabf4a158d4b592dbf28011f3d64560fdd03",
            "20650f7ab8ff990cca0e97a5ce46bb625bf45eced2451a7d483a3859d23e1964",
            "611906738d64d8b4d4f8d9378ccfd811b3a029ca66b6101039008f8554b4c9ba"
    };
    while (!will_quit) {
        int a;
        std::cin >> a;
        if (a == 0) {
            will_quit = true;
            continue;
        }
        if (a == 1)
            c.ping_server();
        if (a == 2)
            c.broadcast_message();
        if (a == 3) {
            blcl::net::message<MsgType> msg;
            msg.header.id = MsgType::EnterMap;
            c.send(msg);
        }
        if (a == 4) {
            blcl::net::message<MsgType> msg;
            int tmp; std::cin >> tmp;
            msg.header.id = MsgType::MapHash;
            const char* hash = map_hashes[tmp + 1].c_str();
            msg.write(hash, strlen(hash) + 1);
            c.send(msg);
        }
        if (a == 5) {
            blcl::net::message<MsgType> msg;
//            blcl::net::message<MsgType> msg = blcl::net::message<MsgType>();
            msg.header.id = MsgType::ExitMap;
            std::cout << msg.header.size << " " << msg.body.size() << std::endl;
            c.send(msg);
        }
    }
    if (thread.joinable())
        thread.join();
}