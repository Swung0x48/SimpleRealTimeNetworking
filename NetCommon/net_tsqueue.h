#ifndef NETCLIENT_NET_TSQUEUE_H
#define NETCLIENT_NET_TSQUEUE_H
#include "net_common.h"

namespace blcl::net {
    template <typename T>
    class tsqueue {
    public:
        tsqueue() = default;
        tsqueue(const tsqueue<T>&) = delete; // explicitly delete copy constructor
        virtual ~tsqueue() { clear(); }

    protected:
        std::mutex mtx_;
        std::deque<T> raw_deque_;

    public:
        const T& front() {
            std::scoped_lock lock(mtx_);
            return raw_deque_.front();
        }

        const T& back() {
            std::scoped_lock lock(mtx_);
            return raw_deque_.back();
        }

        void push_front(const T& item) {
            std::scoped_lock lock(mtx_);
            raw_deque_.emplace_front(std::move(item));
        }

        void push_back(const T& item) {
            std::scoped_lock lock(mtx_);
            raw_deque_.emplace_back(std::move(item));
        }

        bool empty() {
            std::scoped_lock lock(mtx_);
            return raw_deque_.empty();
        }

        size_t size() {
            std::scoped_lock lock(mtx_);
            return raw_deque_.size();
        }

        void clear() {
            std::scoped_lock lock(mtx_);
            raw_deque_.clear();
        }

        T pop_front() {
            std::scoped_lock lock(mtx_);
            auto t = std::move(raw_deque_.front());
            raw_deque_.pop_front();
            return t;
        }

        T pop_back() {
            std::scoped_lock lock(mtx_);
            auto t = std::move(raw_deque_.back());
            raw_deque_.pop_back();
            return t;
        }
    };
}

#endif //NETCLIENT_NET_TSQUEUE_H
