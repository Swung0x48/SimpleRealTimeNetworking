#include <iostream>
#include <blcl_net.h>
#include <unordered_map>

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

struct ClientData {
    std::string username;
    std::string map_hash;
};

class CustomServer: public blcl::net::server_interface<MsgType> {
private:
    std::unordered_map<std::string, uint64_t> fail2ban_counter_;
    uint64_t max_fail_attempt = 10;
    std::unordered_map<std::shared_ptr<blcl::net::connection<MsgType>>, ClientData> online_clients_;
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<blcl::net::connection<MsgType>>>> clients_in_map_;  // maphash, id
    uint32_t max_username_length_ = 30;

    bool is_banned(const std::shared_ptr<blcl::net::connection<MsgType>>& client) {
        if (fail2ban_counter_[client->get_endpoint().address().to_string()] > max_fail_attempt)
            return true;

        ++fail2ban_counter_[client->get_endpoint().address().to_string()];
        return false;
    }
public:
    explicit CustomServer(uint16_t port) : blcl::net::server_interface<MsgType>(port) {

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
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::ClientDisconnect;
        msg.write(online_clients_[client].username.c_str(), online_clients_[client].username.length() + 1);
        msg << client->get_id();
        broadcast_message(msg, get_online_clients(), client, true);
        online_clients_.erase(client);
    }

    void on_client_validated(std::shared_ptr<blcl::net::connection<MsgType>> client) override {
        fail2ban_counter_[client->get_endpoint().address().to_string()] = 0;
        blcl::net::message<MsgType> msg;
        msg.header.id = MsgType::UsernameReq;
        msg << max_username_length_;
        client->send(msg);
    }

    std::unordered_set<std::shared_ptr<blcl::net::connection<MsgType>>> get_clients_in_map(const std::string& map_hash) {
        if (map_hash.empty())
            return std::unordered_set<std::shared_ptr<blcl::net::connection<MsgType>>>();
        return clients_in_map_.at(map_hash);
    }

    const std::string& get_map_hash(const std::shared_ptr<blcl::net::connection<MsgType>>& client) const {
        return online_clients_.at(client).map_hash;
    }

    void on_message(std::shared_ptr<blcl::net::connection<MsgType>> client, blcl::net::message<MsgType>& msg) override {
        switch (msg.header.id) {
            case MsgType::ServerPing: {
                client->send(msg);
                break;
            }
            case MsgType::BallState: {
                uint64_t client_id = client->get_id();
                msg << client_id;
                broadcast_message(msg, get_clients_in_map(get_map_hash(client)), client, true);
                break;
            }
            case MsgType::Username: {
                uint8_t username[msg.size()];
                std::memcpy(username, msg.body.data(), msg.size());
                ClientData data;
                data.username = std::string(reinterpret_cast<const char *>(username));
                online_clients_[client] = data;
                std::cout << "[INFO] " << data.username << " joined the server." << std::endl;

                msg.header.id = MsgType::Username;
                client->send(msg);
                break;
            }
            case MsgType::EnterMap: {
                msg.clear();
                msg.header.id = MsgType::MapHashReq;
                client->send(msg);
                break;
            }
            case MsgType::MapHash: {
                // Assuming user is in online_clients_, update its map hash
                std::string hash = std::string(reinterpret_cast<const char*>(msg.body.data()));
                online_clients_[client].map_hash = hash;
                clients_in_map_[hash].emplace(client);

                msg.clear();
                msg.header.id = MsgType::MapHashAck;
                client->send(msg);
                break;
            }
            case MsgType::FinishLevel: {
                broadcast_message(msg, get_clients_in_map(get_map_hash(client)), client, true);
                break;
            }
            case MsgType::ExitMap: {
                broadcast_message(msg, get_clients_in_map(get_map_hash(client)), client, true);
                clients_in_map_[online_clients_[client].map_hash].erase(client);
                online_clients_[client].map_hash = "";
                break;
            }
            default: {
                std::cerr << "[ERR]: Unknown Message ID:" << (uint32_t) msg.header.id << std::endl;
                break;
            }
        }
    }
};

int main() {
    CustomServer server(60000);
    server.start();
    while (true) {
        server.update(64);
    }

    return 0;
}
