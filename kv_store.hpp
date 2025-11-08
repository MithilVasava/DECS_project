#pragma once
#include <string>
#include <optional>
#include "thread_pool.hpp"
#include "lru.hpp"

class KVStore {
public:
  KVStore(DbPool& pool, size_t cache_size);
  bool put(const std::string& key, const std::string& value);
  std::optional<std::string> get(const std::string& key);
  bool erase(const std::string& key);

private:
  DbPool& pool_;
  LRUCache cache_;
};
