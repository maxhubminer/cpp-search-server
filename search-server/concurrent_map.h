#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <cmath>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : maps_(bucket_count), mutexes_(bucket_count) {
    }

    Access operator[](const Key& key) {
        //uint64_t key_unsigned = static_cast<uint64_t>(key);
        int map_index = (key >= 0 ? key : -key) % maps_.size();

        return { std::lock_guard(mutexes_[map_index]), maps_[map_index][key] };
    }

    void Erase(const Key& key) {
        int map_index = (key >= 0 ? key : -key) % maps_.size();

        maps_[map_index].erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {

        std::map<Key, Value> result_map;

        for (unsigned int i = 0; i < maps_.size(); ++i) {
            std::lock_guard<std::mutex> guard(mutexes_[i]);
            for (const auto& [key, val] : maps_[i]) {
                result_map[key] += val;
            }
            //result_map.merge(map);
        }

        return result_map;

    }

private:
    std::vector<std::map<Key, Value>> maps_;
    std::vector<std::mutex> mutexes_;
};