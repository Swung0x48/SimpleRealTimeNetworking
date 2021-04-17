#ifndef SIMPLENETWORKING_NET_MESSAGE_H
#define SIMPLENETWORKING_NET_MESSAGE_H

#include "net_common.h"
namespace blcl::net {
    template <typename T>
    struct message_header {
        T id {};
        uint32_t size = 0;
    };

    template <typename T>
    struct message {
        message_header<T> header {};
        std::vector<uint8_t> body;

        size_t size() const {
            return sizeof(message_header<T>) + body.size();
        }

        friend std::ostream& operator<<(std::ostream& os, const message<T>& msg) {
            os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
            return os;
        }

        template<typename Data>
        friend message<T>& operator<<(message<T>& msg, const Data& data) {
            static_assert(std::is_standard_layout<Data>::value,
                    "Type of data is not in standard layout thus not able to be serialized.");

            size_t i = msg.body.size();
            std::cout << sizeof(Data) << " " << sizeof(data) << "\n";
            // Resize to vector by the size of data being pushed
            msg.body.resize(msg.body.size() + sizeof(data));
            // Copy the data into the newly allocated vector space
            std::memcpy(msg.body.data() + i, &data, sizeof(data));
            // Recalculate msg size
            msg.header.size = msg.size();

            // Return the resulting msg so that it could be chain-called.
            return msg;
        }

        template <typename Data>
        friend message<T>& operator>>(message<T>& msg, Data& data) {
            static_assert(std::is_standard_layout<Data>::value,
                          "Type of data is not in standard layout thus not able to be deserialized.");

            size_t i = msg.body.size() - sizeof(Data);
            // Copy the data from the vector to the variable
            std::memcpy(&data, msg.body.data() + i, sizeof(Data));
            // Resize the vector, shrink the vector size by read-out bytes, and reset the end position
            msg.body.resize(i);
            // Recalculate msg size
            msg.header.size = msg.size();

            // Return the resulting msg so that it could be chain-called.
            return msg;
        }
    };

    // Forward declare 'connection' class
    template <typename Data>
    class connection;

    template <typename T>
    struct owned_message
    {
        std::shared_ptr<connection<T>> remote = nullptr;
        message<T> msg;

        friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
        {
            os << msg.msg;
            return os;
        }
    };
}

#endif //SIMPLENETWORKING_NET_MESSAGE_H
