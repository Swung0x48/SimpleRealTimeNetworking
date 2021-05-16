#ifndef NETCLIENT_NET_CONNECTION_H
#define NETCLIENT_NET_CONNECTION_H

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace blcl::net {
    template<typename T>
    class server_interface;

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
        {
            owner_type_ = parent;
            if (owner_type_ == owner::server) {
                checksum_out_ = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
                expected_checksum_ = encode(checksum_out_);
            } else {
                checksum_in_ = 0;
                checksum_out_ = 0;
            }

        }
        virtual ~connection() = default;

        uint32_t get_id() const {
            return id_;
        }

        bool is_validated() const {
            return validated_;
        }

        void connect_to_client(blcl::net::server_interface<T>* server, uint32_t uid = 0) {
            if (owner_type_ == owner::server) {
                if (socket_.is_open()) {
                    id_ = uid;
                    write_validation();
                    read_validation(server);
//                    read_header();
                }
            }
        }

        void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints) {
            if (owner_type_ == owner::client) {
                asio::async_connect(socket_, endpoints,
                    [this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint) {
                        if (!ec) {
//                            read_header();
                            read_validation();
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
                    bool writing_message = !outgoing_messages_.empty();
                    outgoing_messages_.push_back(msg);
                    if (!writing_message)
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
                            // Assert if msg size is gonna exceed MAX_MSG_SIZE. If so, log it (for now).
                            if (current_incoming_message_.header.size > MAX_MSG_SIZE) {
                                std::cout << "[WARN]: " << "A message exceeded MAX_MSG_SIZE = " << MAX_MSG_SIZE << "." << std::endl
                                          << "[WARN]: It's gonna allocate " << current_incoming_message_.header.size << "bytes." << std::endl
                                          << "[WARN]: Its ID: " << (uint32_t) current_incoming_message_.header.id << std::endl;
                            }
                            current_incoming_message_.body.resize(current_incoming_message_.header.size);
                            read_body();
                        } else {
                            add_to_incoming_messages_queue();
                        }
                    } else {
                        std::cout << "[WARN] " << id_ << ": Read header failed.\n";
                        std::cout << "[WARN] " << id_ << ": " << ec.message() << "\n";
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
                        std::cout << "[WARN] " << id_ << ": Read body failed.\n";
                        std::cout << "[WARN] " << id_ << ": " << ec.message() << "\n";
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
                        std::cout << "[WARN] " << id_ << ": Write header failed.\n";
                        std::cout << "[WARN] " << id_ << ": " << ec.message() << "\n";
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
                        std::cout << "[WARN] " << id_ << ": Write body failed.\n";
                        std::cout << "[WARN] " << id_ << ": " << ec.message() << "\n";
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

        uint64_t encode(uint64_t bin) {
            auto* slice = reinterpret_cast<uint8_t *>(&bin);
            for (int i = 0; i < sizeof(bin) / sizeof(uint8_t); i++) {
                slice[i] = (slice[i] << 3 | slice[i] >> 5);
                slice[i] = -(slice[i] ^ uint8_t(0xAF));
            }
            return bin;
        }

        // async
        void write_validation() {
            asio::async_write(socket_, asio::buffer(&checksum_out_, sizeof(uint64_t)),
                [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (owner_type_ == owner::client)
                            read_header();
                    } else {
                        socket_.close();
                    }
            });
        }

        // async
        void read_validation(blcl::net::server_interface<T>* server = nullptr) {
            asio::async_read(socket_, asio::buffer(&checksum_in_, sizeof(uint64_t)),
                [this, server](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (owner_type_ == owner::server) {
                            if (checksum_in_ == expected_checksum_) {
                                std::cout << "[INFO] Challenge-response passed." << std::endl;
                                validated_ = true;
                                server->on_client_validated(this->shared_from_this());

                                read_header();
                            } else {
                                std::cout << "[WARN] Client disconnected: challenge-reponse failed." << std::endl;
                                socket_.close();
                            }
                        } else {
                            checksum_out_ = encode(checksum_in_);
                            write_validation();
                        }
                    } else {
                        std::cout << "[WARN] Client disconnected on reading challenge-response." << std::endl;
                        socket_.close();
                    }
            });
        }

    protected:
        asio::ip::tcp::socket socket_;
        asio::io_context& asio_context_;
        tsqueue<message<T>> outgoing_messages_;
        tsqueue<owned_message<T>>& incoming_messages_;
        message<T> current_incoming_message_;
        owner owner_type_ = owner::server;
        uint32_t id_ = 0;
        bool validated_ = false;

        uint64_t checksum_out_ = 0;
        uint64_t checksum_in_ = 0;
        uint64_t expected_checksum_ = 0;
    };
}

#endif //NETCLIENT_NET_CONNECTION_H
