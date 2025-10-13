#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <sw/redis++/redis++.h>

class RedisCache {
public:
    RedisCache(const std::string& host, int port, const std::string& password, int max_size);
    ~RedisCache();

    bool get(const std::string& text, std::vector<char>& wav_data);
    void put(const std::string& text, const std::vector<char>& wav_data);
    
private:
    std::unique_ptr<sw::redis::Redis> redis_;
    int max_size_;
    std::mutex mutex_;

    std::string makeKey(const std::string& text);
    void evictLRU();
    void updateAccessTime(const std::string& key);
};
