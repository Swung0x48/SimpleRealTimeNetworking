#ifndef SIMPLENETWORKING_NET_COMMON_H
#define SIMPLENETWORKING_NET_COMMON_H

#include <cstdint>
#include <iostream>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

constexpr uint32_t MAX_MSG_SIZE = 512;

#endif //SIMPLENETWORKING_NET_COMMON_H
