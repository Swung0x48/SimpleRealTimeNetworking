#ifndef SIMPLESERVER_NET_TS_UNORDERED_MAP_H
#define SIMPLESERVER_NET_TS_UNORDERED_MAP_H
#include "net_common.h"

namespace blcl::net {
    template <typename K, typename V>
    class ts_unordered_map {
    public:
        ts_unordered_map() = default;
        ts_unordered_map(const ts_unordered_map<K, V>&) = delete; // explicitly delete copy constructor
        virtual ~ts_unordered_map() { clear(); }

    protected:
        std::mutex map_mtx_;
        std::unordered_map<K, V> raw_unordered_map_;
//        std::condition_variable to_update_cv_;
//        std::mutex to_update_mtx_;

    public:
        void insert(const K& key, const V& value) {
            std::scoped_lock lock(map_mtx_);
            raw_unordered_map_[key] = value;
        }

        void insert(const std::pair<K, V>& pair) {
            std::scoped_lock lock(map_mtx_);
            raw_unordered_map_.insert(pair);
        }

        std::optional<V> get(const K& key) {
            std::scoped_lock lock(map_mtx_);
            auto it = raw_unordered_map_.find(key);
            if (it != raw_unordered_map_.end())
                return it->second;
            return {};
        }

        bool empty() {
            std::scoped_lock lock(map_mtx_);
            return raw_unordered_map_.empty();
        }

        size_t size() {
            std::scoped_lock lock(map_mtx_);
            return raw_unordered_map_.size();
        }

        void clear() {
            std::scoped_lock lock(map_mtx_);
            raw_unordered_map_.clear();
        }

//        void wait() {
//            while (empty()) {
//                std::unique_lock<std::mutex> lk(to_update_mtx_);
//                to_update_cv_.wait(lk);
//            }
//        }
    };
}
#endif //SIMPLESERVER_NET_TS_UNORDERED_MAP_H
