#ifndef NETCLIENT_NET_CONNECTION_H
#define NETCLIENT_NET_CONNECTION_H

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace blcl::net {
    template<typename T>
    class connection: public std::enable_shared_from_this<connection<T>> {
    public:
        enum class owner {
            server,
            client
        };

        connection(owner parent, asio::io_context& asio_context, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& incoming_messages)
            : asio_context_(asio_context), socket_(std::move(socket)), incoming_messages_(incoming_messages),
            owner_type_(parent)
        {}
        virtual ~connection() {}

        uint32_t get_id() const {
            return id;
        }

        void connect_to_client(uint32_t uid = 0) {
            if (owner_type_ == owner::server) {
                if (socket_.is_open()) {
                    id = uid;
                    read_header();
                }
            }
        }

        void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints) {
            if (owner_type_ == owner::client) {
                asio::async_connect(socket_, endpoints,
                    [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
                        if (!ec) {
                            read_header();
                        }
                });
            }
        }

        void disconnect() {
            if (is_connected())
                asio::post(asio_context_, [this]() { socket_.close(); });
        }

        bool is_connected() const {
            return socket_.is_open();
        }

        void send(const message<T>& msg) {
            asio::post(asio_context_,
                [this, msg]() {
                    bool to_write_message = !outgoing_messages_.empty();
                    outgoing_messages_.push_back(msg);
                    if (to_write_message)
                        write_header();
            });
        }

    private:
        // async
        void read_header() {
            asio::async_read(socket_, asio::buffer(&current_incoming_message_.header, sizeof(message_header<T>)),
                [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (current_incoming_message_.header.size > 0) {
                            current_incoming_message_.body.resize(current_incoming_message_.header.size);
                            read_body();
                        } else {
                            add_to_incoming_messages_queue();
                        }
                    } else {
                        std::cout << "[WARN] " << id << ": Read header failed.\n";
                        std::cout << "[WARN] " << id << ": " << ec.message() << "\n";
                        socket_.close();
                    }
            });
        }

        // async
        void read_body() {
            asio::async_read(socket_, asio::buffer(current_incoming_message_.body.data(), current_incoming_message_.size()),
                [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        add_to_incoming_messages_queue();
                    } else {
                        std::cout << "[WARN] " << id << ": Read body failed.\n";
                        std::cout << "[WARN] " << id << ": " << ec.message() << "\n";
                        socket_.close();
                    }
            });
        }

        // async
        void write_header() {
            asio::async_write(socket_, asio::buffer(&outgoing_messages_.front().header, sizeof(message_header<T>)),
                [this](asio::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (outgoing_messages_.front().body.size() > 0) {
                            write_body();
                        } else {
                            outgoing_messages_.pop_front();
                            if (!outgoing_messages_.empty())
                                write_header();
                        }
                    } else {
                        std::cout << "[WARN] " << id << ": Write header failed.\n";
                        std::cout << "[WARN] " << id << ": " << ec.message() << "\n";
                        socket_.close();
                    }
            });
        }

        // async
        void write_body() {
            asio::async_write(socket_, asio::buffer(outgoing_messages_.front().body.data(), outgoing_messages_.front().body.size()),
                [this](asio::error_code ec, std::size_t length) {
                    if (!ec) {
                        outgoing_messages_.pop_front();
                        if (!outgoing_messages_.empty())
                            write_header();
                    } else {
                        std::cout << "[WARN] " << id << ": Write body failed.\n";
                        std::cout << "[WARN] " << id << ": " << ec.message() << "\n";
                        socket_.close();
                    }
            });
        }

        void add_to_incoming_messages_queue() {
            if (owner_type_ == owner::server)
                incoming_messages_.push_back({ this->shared_from_this(), current_incoming_message_ });
            else
                incoming_messages_.push_back({ nullptr, current_incoming_message_ });

            read_header();
        }
    protected:
        asio::ip::tcp::socket socket_;
        asio::io_context& asio_context_;
        tsqueue<message<T>> outgoing_messages_;
        tsqueue<owned_message<T>>& incoming_messages_;
        message<T> current_incoming_message_;
        owner owner_type_ = owner::server;
        uint32_t id = 0;
    };
}

#endif //NETCLIENT_NET_CONNECTION_H
