#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "redis_cache.h"
#include <thread>
#include <chrono>

class RedisCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        host = "localhost";
        port = 6379;
        password = "";
    }
    
    std::string host;
    int port;
    std::string password;
};

TEST_F(RedisCacheTest, BasicPutAndGet) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> wav_data = {'R', 'I', 'F', 'F', 0x00, 0x01, 0x02, 0x03};
    std::string text = "Hello world";
    
    cache.put(text, wav_data);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(text, retrieved));
    EXPECT_EQ(wav_data, retrieved);
}

TEST_F(RedisCacheTest, GetNonExistentKey) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> retrieved;
    EXPECT_FALSE(cache.get("non_existent_key", retrieved));
    EXPECT_TRUE(retrieved.empty());
}

TEST_F(RedisCacheTest, OverwriteExistingKey) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> wav1 = {'A', 'B', 'C'};
    std::vector<char> wav2 = {'X', 'Y', 'Z'};
    std::string text = "Same text";
    
    cache.put(text, wav1);
    cache.put(text, wav2);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(text, retrieved));
    EXPECT_EQ(wav2, retrieved);
}

TEST_F(RedisCacheTest, LRUEviction) {
    RedisCache cache(host, port, password, 2);
    
    std::vector<char> wav1 = {'A'};
    std::vector<char> wav2 = {'B'};
    std::vector<char> wav3 = {'C'};
    
    cache.put("text1", wav1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    cache.put("text2", wav2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    cache.put("text3", wav3);
    
    std::vector<char> retrieved;
    EXPECT_FALSE(cache.get("text1", retrieved));
    
    ASSERT_TRUE(cache.get("text2", retrieved));
    EXPECT_EQ(wav2, retrieved);
    
    ASSERT_TRUE(cache.get("text3", retrieved));
    EXPECT_EQ(wav3, retrieved);
}

TEST_F(RedisCacheTest, EmptyWavData) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> empty_wav;
    std::string text = "Empty audio";
    
    cache.put(text, empty_wav);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(text, retrieved));
    EXPECT_TRUE(retrieved.empty());
}

TEST_F(RedisCacheTest, LargeWavData) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> large_wav(1024 * 1024, 'X');
    std::string text = "Large audio file";
    
    cache.put(text, large_wav);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(text, retrieved));
    EXPECT_EQ(large_wav.size(), retrieved.size());
    EXPECT_EQ(large_wav, retrieved);
}

TEST_F(RedisCacheTest, BinaryWavData) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> binary_wav;
    for (int i = 0; i < 256; ++i) {
        binary_wav.push_back(static_cast<char>(i));
    }
    
    std::string text = "Binary data";
    cache.put(text, binary_wav);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(text, retrieved));
    EXPECT_EQ(binary_wav, retrieved);
}

TEST_F(RedisCacheTest, SpecialCharactersInText) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> wav = {'W', 'A', 'V'};
    std::string special_text = "Hello\nWorld\tWith\"Quotes\"And'Apostrophes'";
    
    cache.put(special_text, wav);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(special_text, retrieved));
    EXPECT_EQ(wav, retrieved);
}

TEST_F(RedisCacheTest, UnicodeText) {
    RedisCache cache(host, port, password, 10);
    
    std::vector<char> wav = {'W', 'A', 'V'};
    std::string unicode_text = "Hello 世界 🌍";
    
    cache.put(unicode_text, wav);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(unicode_text, retrieved));
    EXPECT_EQ(wav, retrieved);
}

TEST_F(RedisCacheTest, ThreadSafety) {
    RedisCache cache(host, port, password, 100);
    
    auto worker = [&cache](int thread_id) {
        for (int i = 0; i < 10; ++i) {
            std::string text = "Thread_" + std::to_string(thread_id) + "_Item_" + std::to_string(i);
            std::vector<char> wav = {static_cast<char>('A' + thread_id), static_cast<char>('0' + i)};
            
            cache.put(text, wav);
            
            std::vector<char> retrieved;
            if (cache.get(text, retrieved)) {
                EXPECT_EQ(wav, retrieved);
            }
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

TEST_F(RedisCacheTest, MultipleInstances) {
    RedisCache cache1(host, port, password, 10);
    RedisCache cache2(host, port, password, 10);
    
    std::vector<char> wav = {'S', 'H', 'A', 'R', 'E', 'D'};
    std::string text = "Shared data";
    
    cache1.put(text, wav);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache2.get(text, retrieved));
    EXPECT_EQ(wav, retrieved);
}
