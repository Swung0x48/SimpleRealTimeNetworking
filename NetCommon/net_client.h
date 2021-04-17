#ifndef NETCLIENT_NET_CLIENT_H
#define NETCLIENT_NET_CLIENT_H

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace blcl::net {
    template<typename T>
    class client_interface {
    public:
        client_interface(): socket_(context_) {

        }

        virtual ~client_interface() {
            disconnect();
        }

    public:
        bool connect(const std::string& host, const uint16_t port) {
            try {
                asio::ip::tcp::resolver resolver(context_);
                auto endpoints = resolver.resolve(host, std::to_string(port));

                connection_ = std::make_unique<connection<T>>(
                        connection<T>::owner::client, context_, asio::ip::tcp::socket(context_), incoming_messages_);
                connection_->connect_to_server(endpoints);
                ctx_thread_ = std::thread([this]() { context_.run(); });
            } catch (std::exception& e) {
                std::cerr << "Client exception: " << e.what() << '\n';
                return false;
            }

            return false;
        }

        void disconnect() {
            if (is_connected())
                connection_->disconnect();

            context_.stop();
            if (ctx_thread_.joinable())
                ctx_thread_.join();
            connection_.release();
        }

        bool is_connected() {
            if (connection_) {
                return connection_->is_connected();
            } else {
                return false;
            }
        }

        void send(const message<T>& msg) {
            if (is_connected())
                connection_->send(msg);
        }

        tsqueue<owned_message<T>>& get_incoming_messages() {
            return incoming_messages_;
        }
    protected:
        asio::io_context context_;
        std::thread ctx_thread_;
        asio::ip::tcp::socket socket_;
        std::unique_ptr<connection<T>> connection_;
    private:
        tsqueue<owned_message<T>> incoming_messages_;
    };
}

#endif //NETCLIENT_NET_CLIENT_H
