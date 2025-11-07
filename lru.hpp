#pragma once
#include <unordered_map>
#include <list>
#include <string>
#include <mutex>
#include <optional>


class LRU{
    private:
        size_t cap;
        std::list<std::pair<std::string, std::string>> items;
        std::unordered_map<std::string, decltype(items.begin())> map;
        std::mutex mu;

    public:
        explicit LRU(size_t capacity): cap(capacity) {}

        std::optional<std::string> get(const std::string &key){
            std::lock_guard<std::mutex> g(mu);
            auto it = map.find(key);
            if(it==map.end()){
                return std::nullopt;
            }
            items.splice(items.begin(), items, it->second);
            return it->second->second;
        }

        void put(const std::string &key, const std::string &value){
            std::lock_guard<std::mutex> g(mu);
            auto it = map.find(key);
            if(it!=map.end()){
                it->second->second = value;
                items.splice(items.begin(), items, it->second);
                return;
            }
            items.emplace_front(key, value);
            map[key] = items.begin();
            if(map.size() > cap){
                auto &back = items.back().first;
                map.erase(back);
                items.pop_back();
            }
        }

        void erase(const std::string &key){
            std::lock_guard<std::mutex> g(mu);
            auto it = map.find(key);
            if(it==map.end()){
                return;
            }
            items.erase(it->second);
            map.erase(it);
        }


};