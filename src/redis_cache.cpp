#include "redis_cache.h"
#include <spdlog/spdlog.h>
#include <chrono>

RedisCache::RedisCache(const std::string& host, int port, const std::string& password, int max_size)
    : max_size_(max_size) {
    
    sw::redis::ConnectionOptions opts;
    opts.host = host;
    opts.port = port;
    if (!password.empty()) {
        opts.password = password;
    }
    opts.socket_timeout = std::chrono::milliseconds(100);
    
    redis_ = std::make_unique<sw::redis::Redis>(opts);
    spdlog::info("Redis cache initialized with max size: {}", max_size_);
}

RedisCache::~RedisCache() = default;

std::string RedisCache::makeKey(const std::string& text) {
    return "tts:wav:" + text;
}

bool RedisCache::get(const std::string& text, std::vector<char>& wav_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::string key = makeKey(text);
        auto val = redis_->get(key);
        
        if (val) {
            wav_data.assign(val->begin(), val->end());
            updateAccessTime(key);
            spdlog::debug("Cache HIT for text: {}", text.substr(0, 50));
            return true;
        }
        
        spdlog::debug("Cache MISS for text: {}", text.substr(0, 50));
        return false;
    } catch (const sw::redis::Error& e) {
        spdlog::error("Redis get error: {}", e.what());
        return false;
    }
}

void RedisCache::put(const std::string& text, const std::vector<char>& wav_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        auto current_size = redis_->zcard("tts:cache:lru");
        
        if (current_size >= static_cast<long long>(max_size_)) {
            evictLRU();
        }
        
        std::string key = makeKey(text);
        std::string wav_str(wav_data.begin(), wav_data.end());
        
        redis_->set(key, wav_str);
        updateAccessTime(key);
        
        spdlog::debug("Cached WAV for text: {} (size: {} bytes)", text.substr(0, 50), wav_data.size());
    } catch (const sw::redis::Error& e) {
        spdlog::error("Redis put error: {}", e.what());
    }
}

void RedisCache::updateAccessTime(const std::string& key) {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    redis_->zadd("tts:cache:lru", key, static_cast<double>(now));
}

void RedisCache::evictLRU() {
    try {
        std::vector<std::string> keys;
        redis_->zrange("tts:cache:lru", 0, 0, std::back_inserter(keys));
        
        if (!keys.empty()) {
            const std::string& lru_key = keys[0];
            redis_->del(lru_key);
            redis_->zrem("tts:cache:lru", lru_key);
            spdlog::debug("Evicted LRU key: {}", lru_key);
        }
    } catch (const sw::redis::Error& e) {
        spdlog::error("Redis eviction error: {}", e.what());
    }
}
