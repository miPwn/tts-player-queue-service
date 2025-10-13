#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "config.h"
#include "redis_cache.h"
#include "rabbitmq_client.h"
#include "audio_player.h"
#include "api_server.h"
#include <thread>
#include <chrono>
#include <httplib.h>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("RABBITMQ_HOST", "localhost", 1);
        setenv("RABBITMQ_PORT", "5672", 1);
        setenv("REDIS_HOST", "localhost", 1);
        setenv("REDIS_PORT", "6379", 1);
        setenv("API_HOST", "127.0.0.1", 1);
        setenv("API_PORT", "18081", 1);
        setenv("CACHE_SIZE", "5", 1);
    }
    
    std::vector<char> createMinimalWav() {
        std::vector<char> wav;
        wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
        uint32_t size = 36;
        wav.insert(wav.end(), {char(size & 0xFF), char((size >> 8) & 0xFF), 
                               char((size >> 16) & 0xFF), char((size >> 24) & 0xFF)});
        wav.insert(wav.end(), {'W', 'A', 'V', 'E'});
        wav.insert(wav.end(), {'f', 'm', 't', ' '});
        uint32_t fmt_size = 16;
        wav.insert(wav.end(), {char(fmt_size & 0xFF), char((fmt_size >> 8) & 0xFF),
                               char((fmt_size >> 16) & 0xFF), char((fmt_size >> 24) & 0xFF)});
        wav.insert(wav.end(), {0x01, 0x00, 0x02, 0x00});
        uint32_t sr = 44100;
        wav.insert(wav.end(), {char(sr & 0xFF), char((sr >> 8) & 0xFF),
                               char((sr >> 16) & 0xFF), char((sr >> 24) & 0xFF)});
        uint32_t br = 176400;
        wav.insert(wav.end(), {char(br & 0xFF), char((br >> 8) & 0xFF),
                               char((br >> 16) & 0xFF), char((br >> 24) & 0xFF)});
        wav.insert(wav.end(), {0x04, 0x00, 0x10, 0x00});
        wav.insert(wav.end(), {'d', 'a', 't', 'a'});
        uint32_t data_size = 0;
        wav.insert(wav.end(), {char(data_size & 0xFF), char((data_size >> 8) & 0xFF),
                               char((data_size >> 16) & 0xFF), char((data_size >> 24) & 0xFF)});
        return wav;
    }
};

TEST_F(IntegrationTest, ConfigLoadsFromEnvironment) {
    Config config;
    
    EXPECT_EQ(config.rabbitmq_host, "localhost");
    EXPECT_EQ(config.rabbitmq_port, 5672);
    EXPECT_EQ(config.redis_host, "localhost");
    EXPECT_EQ(config.redis_port, 6379);
    EXPECT_EQ(config.api_host, "127.0.0.1");
    EXPECT_EQ(config.api_port, 18081);
    EXPECT_EQ(config.cache_size, 5);
}

TEST_F(IntegrationTest, RedisCacheIntegration) {
    Config config;
    RedisCache cache(config.redis_host, config.redis_port, config.redis_password, config.cache_size);
    
    std::vector<char> wav_data = createMinimalWav();
    std::string text = "Integration test audio";
    
    cache.put(text, wav_data);
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get(text, retrieved));
    EXPECT_EQ(wav_data, retrieved);
}

TEST_F(IntegrationTest, RabbitMQPublishConsume) {
    Config config;
    
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(
        config.rabbitmq_host,
        config.rabbitmq_port,
        config.rabbitmq_user,
        config.rabbitmq_password,
        config.rabbitmq_vhost,
        config.rabbitmq_queue,
        callback
    );
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Integration test";
    job.wav_data = createMinimalWav();
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.text, received_job.text);
}

TEST_F(IntegrationTest, APIToCache) {
    Config config;
    RedisCache cache(config.redis_host, config.redis_port, config.redis_password, config.cache_size);
    
    auto callback = [&](const TTSRequest& req) {
        cache.put(req.text, req.wav_data);
    };
    
    ApiServer server(config.api_host, config.api_port, callback);
    std::thread server_thread([&]() {
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    httplib::Client client(config.api_host, config.api_port);
    httplib::MultipartFormDataItems items = {
        {"text", "API to Cache test", "", ""},
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };
    
    auto res = client.Post("/api/tts/play", items);
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::vector<char> retrieved;
    ASSERT_TRUE(cache.get("API to Cache test", retrieved));
    EXPECT_FALSE(retrieved.empty());
    
    server.stop();
    server_thread.join();
}

TEST_F(IntegrationTest, EndToEndWithoutAudioPlayback) {
    Config config;
    RedisCache cache(config.redis_host, config.redis_port, config.redis_password, config.cache_size);
    
    std::atomic<int> playback_count{0};
    
    auto rabbitmq_callback = [&](const PlaybackJob& job) {
        playback_count++;
    };
    
    RabbitMQClient rabbitmq(
        config.rabbitmq_host,
        config.rabbitmq_port,
        config.rabbitmq_user,
        config.rabbitmq_password,
        config.rabbitmq_vhost,
        "integration_test_queue",
        rabbitmq_callback
    );
    
    auto api_callback = [&](const TTSRequest& req) {
        std::vector<char> cached;
        if (!cache.get(req.text, cached)) {
            cache.put(req.text, req.wav_data);
        }
        
        PlaybackJob job;
        job.text = req.text;
        job.wav_data = req.wav_data;
        rabbitmq.publishJob(job);
    };
    
    ApiServer server(config.api_host, config.api_port, api_callback);
    std::thread server_thread([&]() {
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    httplib::Client client(config.api_host, config.api_port);
    httplib::MultipartFormDataItems items = {
        {"text", "End to end test", "", ""},
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };
    
    auto res = client.Post("/api/tts/play", items);
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_EQ(1, playback_count.load());
    
    std::vector<char> cached;
    ASSERT_TRUE(cache.get("End to end test", cached));
    
    server.stop();
    server_thread.join();
}

TEST_F(IntegrationTest, CacheHitSkipsStorage) {
    Config config;
    RedisCache cache(config.redis_host, config.redis_port, config.redis_password, config.cache_size);
    
    std::vector<char> wav = createMinimalWav();
    cache.put("Cached text", wav);
    
    std::atomic<bool> stored{false};
    
    auto api_callback = [&](const TTSRequest& req) {
        std::vector<char> cached;
        if (!cache.get(req.text, cached)) {
            cache.put(req.text, req.wav_data);
            stored = true;
        }
    };
    
    ApiServer server(config.api_host, config.api_port, api_callback);
    std::thread server_thread([&]() {
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    httplib::Client client(config.api_host, config.api_port);
    httplib::MultipartFormDataItems items = {
        {"text", "Cached text", "", ""},
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };
    
    auto res = client.Post("/api/tts/play", items);
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_FALSE(stored);
    
    server.stop();
    server_thread.join();
}
