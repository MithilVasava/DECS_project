#pragma once
#include <unordered_map>
#include <list>
#include <string>
#include <mutex>
#include <optional>

class LRUCache {
public:
  explicit LRUCache(size_t capacity) : cap_(capacity) {} //initialize cache acc to size

  std::optional<std::string> get(const std::string& key) {
    std::lock_guard<std::mutex> g(mu_);
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    items_.splice(items_.begin(), items_, it->second);
    return it->second->second;
  }

  void put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> g(mu_);
    auto it = map_.find(key);
    if (it != map_.end()) {
      it->second->second = value;
      items_.splice(items_.begin(), items_, it->second);
      return;
    }
    items_.emplace_front(key, value);
    map_[key] = items_.begin();
    if (map_.size() > cap_) {
      const auto& old = items_.back().first;
      map_.erase(old);
      items_.pop_back();
    }
  }

  void erase(const std::string& key) {
    std::lock_guard<std::mutex> g(mu_);
    auto it = map_.find(key);
    if (it == map_.end()) return;
    items_.erase(it->second);
    map_.erase(it);
  }

private:
  size_t cap_;
  std::list<std::pair<std::string,std::string>> items_;
  std::unordered_map<std::string,decltype(items_.begin())> map_;
  std::mutex mu_;
};
