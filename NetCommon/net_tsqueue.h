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
        std::mutex queue_mtx_;
        std::deque<T> raw_deque_;
        std::condition_variable to_update_cv_;
        std::mutex to_update_mtx_;

    public:
        const T& front() {
            std::scoped_lock lock(queue_mtx_);
            return raw_deque_.front();
        }

        const T& back() {
            std::scoped_lock lock(queue_mtx_);
            return raw_deque_.back();
        }

        void push_front(const T& item) {
            std::scoped_lock lock(queue_mtx_);
            raw_deque_.emplace_front(std::move(item));

            std::unique_lock<std::mutex> lk(to_update_mtx_);
            to_update_cv_.notify_one();
        }

        void push_back(const T& item) {
            std::scoped_lock lock(queue_mtx_);
            raw_deque_.emplace_back(std::move(item));

            std::unique_lock<std::mutex> lk(to_update_mtx_);
            to_update_cv_.notify_one();
        }

        bool empty() {
            std::scoped_lock lock(queue_mtx_);
            return raw_deque_.empty();
        }

        size_t size() {
            std::scoped_lock lock(queue_mtx_);
            return raw_deque_.size();
        }

        void clear() {
            std::scoped_lock lock(queue_mtx_);
            raw_deque_.clear();
        }

        T pop_front() {
            std::scoped_lock lock(queue_mtx_);
            auto t = std::move(raw_deque_.front());
            raw_deque_.pop_front();
            return t;
        }

        T pop_back() {
            std::scoped_lock lock(queue_mtx_);
            auto t = std::move(raw_deque_.back());
            raw_deque_.pop_back();
            return t;
        }

        void wait() {
            while (empty()) {
                std::unique_lock<std::mutex> lk(to_update_mtx_);
                to_update_cv_.wait(lk);
            }
        }
    };
}

#endif //NETCLIENT_NET_TSQUEUE_H
