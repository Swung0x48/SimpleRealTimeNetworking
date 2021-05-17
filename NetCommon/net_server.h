#ifndef NETCLIENT_NET_SERVER_H
#define NETCLIENT_NET_SERVER_H

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace blcl::net {
    template <typename T>
    class server_interface {
    public:
        server_interface(uint16_t port)
            : asio_acceptor_(context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {

        }

        virtual ~server_interface() {
            stop();
        }

        bool start() {
            try {
                wait_for_client_connection();
                ctx_thread_ = std::thread([this]() { context_.run(); });
            } catch (std::exception& e) {
                std::cerr << "[ERR] Exception: " << e.what() << "\n";
                return false;
            }

            std::cout << "[INFO] Server started!\n";
            return true;
        }

        void stop() {
            context_.stop();
            if (ctx_thread_.joinable())
                ctx_thread_.join();

            std::cout << "[INFO] Server Stopped!\n";
        }

        // async
        void wait_for_client_connection() {
            asio_acceptor_.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket socket) {
                        if (!ec) {
                            std::cout << "[INFO] New connection: " << socket.remote_endpoint() << "\n";

                            std::shared_ptr<connection<T>> new_connection =
                                    std::make_shared<connection<T>>(
                                            connection<T>::owner::server, context_, std::move(socket), incoming_messages_);

                            if (on_client_connect(new_connection)) {
                                connections_.push_back(std::move(new_connection));
                                connections_.back()->connect_to_client(this, id_counter_++);
                                std::cout << "[INFO] Connection established. Connection ID: " << connections_.back()->get_id() << '\n';
                            } else {
                                std::cout << "[WARN] Client disconnected on checking preconditions (likely fail2ban).\n";
                            }
                        } else {
                            std::cout << "[WARN] Connection error occurred: " << ec.message() << "\n";
                        }

                        wait_for_client_connection();
                    });
        }

        void send_message_to_client(std::shared_ptr<connection<T>> client, const message<T>& msg) {
            if (client && client->is_connected()) {
                client->send(msg);
                return;
            }

            on_client_disconnect(client);
            client.reset();
            connections_.erase(
                    std::remove(connections_.begin(), connections_.end(), client), connections_.end());
        }

        std::unordered_set<std::shared_ptr<connection<T>>> get_online_clients() {
            bool invalid_client_exists = false;
            std::unordered_set<std::shared_ptr<connection<T>>> clients;
            clients.reserve(connections_.size());

            for (auto& client: connections_) {
                if (client && client->is_connected()) {
                    clients.emplace(client);
                } else {
                    on_client_disconnect(client);
                    client.reset();
                    invalid_client_exists = true;
                }
            }

            if (invalid_client_exists)
                connections_.erase(
                        std::remove(connections_.begin(), connections_.end(), nullptr), connections_.end());

            return clients;
        }

        std::unordered_set<uint64_t> get_online_clients_id() {
            bool invalid_client_exists = false;
            std::unordered_set<uint64_t> clients;
            clients.reserve(connections_.size());

            for (auto& client: connections_) {
                if (client && client->is_connected()) {
                    clients.emplace(client->get_id());
                } else {
                    on_client_disconnect(client);
                    client.reset();
                    invalid_client_exists = true;
                }
            }

            if (invalid_client_exists)
                connections_.erase(
                        std::remove(connections_.begin(), connections_.end(), nullptr), connections_.end());

            return clients;
        }

        void broadcast_message(const message<T>& msg,
                               const std::unordered_set<std::shared_ptr<connection<T>>>& clients_to_send,
                               std::shared_ptr<connection<T>> initiator,
                               bool ignore_initiator = true) {
            bool invalid_client_exists = false;

            for (auto& client: connections_) {
                if (client && client->is_connected()) {
                    if (clients_to_send.find(client) != clients_to_send.end() && // Current client in clients_to_send
                        !(ignore_initiator && client == initiator)) // Not initiator or does not ignore initiator -> true
                        client->send(msg);
                } else {
                    on_client_disconnect(client);
                    client.reset();
                    invalid_client_exists = true;
                }
            }

            if (invalid_client_exists)
                connections_.erase(
                        std::remove(connections_.begin(), connections_.end(), nullptr), connections_.end());
        }

        void update(size_t max_message_count = -1, bool wait = true) {
            if (wait)
                incoming_messages_.wait();

            size_t message_count = 0;
            while (message_count < max_message_count && !incoming_messages_.empty()) {
                auto msg = incoming_messages_.pop_front();
                on_message(msg.remote, msg.msg);
                message_count++;
            }
        }

    protected:
        virtual bool on_client_connect(std::shared_ptr<connection<T>> client) { return false; }
        virtual void on_client_disconnect(std::shared_ptr<connection<T>> client) { }
        virtual void on_message(std::shared_ptr<connection<T>> client, message<T>& msg) { }
    public:
        virtual void on_client_validated(std::shared_ptr<connection<T>> client) { }
    protected:
        tsqueue<owned_message<T>> incoming_messages_;
        std::deque<std::shared_ptr<connection<T>>> connections_;
        asio::io_context context_;
        std::thread ctx_thread_;
        asio::ip::tcp::acceptor asio_acceptor_;
        uint64_t id_counter_ = 10000;
    };
}

#endif //NETCLIENT_NET_SERVER_H
