#pragma once
#include <string>
#include <optional>
#include "thread_pool.hpp"
#include "lru.hpp"

class kv_store{
    public:
        kv_store(Thread_pool &pool, size_t cache_items = 100000);

        bool put(const std::string& key, const std::string& value);
        std::optional<std::string> get(const std::string &key);
        bool erase(const std::string &key);

    private:
        Thread_pool& pool_;
        LRU cache_;
};