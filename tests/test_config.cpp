#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "config.h"
#include <cstdlib>
#include <string>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        unsetenv("RABBITMQ_HOST");
        unsetenv("RABBITMQ_PORT");
        unsetenv("RABBITMQ_USER");
        unsetenv("RABBITMQ_PASSWORD");
        unsetenv("RABBITMQ_VHOST");
        unsetenv("RABBITMQ_QUEUE");
        unsetenv("REDIS_HOST");
        unsetenv("REDIS_PORT");
        unsetenv("REDIS_PASSWORD");
        unsetenv("CACHE_SIZE");
        unsetenv("API_HOST");
        unsetenv("API_PORT");
        unsetenv("PULSEAUDIO_SINK");
    }

    void TearDown() override {
        unsetenv("RABBITMQ_HOST");
        unsetenv("RABBITMQ_PORT");
        unsetenv("REDIS_HOST");
        unsetenv("CACHE_SIZE");
    }
};

TEST_F(ConfigTest, DefaultValues) {
    auto& config = Config::getInstance();
    config.load();

    EXPECT_EQ(config.rabbitmq_host, "localhost");
    EXPECT_EQ(config.rabbitmq_port, 5672);
    EXPECT_EQ(config.rabbitmq_user, "guest");
    EXPECT_EQ(config.rabbitmq_password, "guest");
    EXPECT_EQ(config.rabbitmq_vhost, "/");
    EXPECT_EQ(config.rabbitmq_queue, "tts_playback_queue");

    EXPECT_EQ(config.redis_host, "localhost");
    EXPECT_EQ(config.redis_port, 6379);
    EXPECT_EQ(config.redis_password, "");
    EXPECT_EQ(config.cache_size, 10);

    EXPECT_EQ(config.api_host, "0.0.0.0");
    EXPECT_EQ(config.api_port, 8080);

    EXPECT_EQ(config.pulseaudio_sink, "");
}

TEST_F(ConfigTest, EnvironmentVariableOverride) {
    setenv("RABBITMQ_HOST", "rabbitmq.example.com", 1);
    setenv("RABBITMQ_PORT", "5673", 1);
    setenv("RABBITMQ_USER", "admin", 1);
    setenv("RABBITMQ_PASSWORD", "secret", 1);
    setenv("RABBITMQ_VHOST", "/prod", 1);
    setenv("RABBITMQ_QUEUE", "custom_queue", 1);

    setenv("REDIS_HOST", "redis.example.com", 1);
    setenv("REDIS_PORT", "6380", 1);
    setenv("REDIS_PASSWORD", "redispass", 1);
    setenv("CACHE_SIZE", "100", 1);

    setenv("API_HOST", "127.0.0.1", 1);
    setenv("API_PORT", "9090", 1);

    setenv("PULSEAUDIO_SINK", "custom_sink", 1);

    auto& config = Config::getInstance();
    config.load();

    EXPECT_EQ(config.rabbitmq_host, "rabbitmq.example.com");
    EXPECT_EQ(config.rabbitmq_port, 5673);
    EXPECT_EQ(config.rabbitmq_user, "admin");
    EXPECT_EQ(config.rabbitmq_password, "secret");
    EXPECT_EQ(config.rabbitmq_vhost, "/prod");
    EXPECT_EQ(config.rabbitmq_queue, "custom_queue");

    EXPECT_EQ(config.redis_host, "redis.example.com");
    EXPECT_EQ(config.redis_port, 6380);
    EXPECT_EQ(config.redis_password, "redispass");
    EXPECT_EQ(config.cache_size, 100);

    EXPECT_EQ(config.api_host, "127.0.0.1");
    EXPECT_EQ(config.api_port, 9090);

    EXPECT_EQ(config.pulseaudio_sink, "custom_sink");
}

TEST_F(ConfigTest, InvalidPortDefaultsToStandard) {
    setenv("RABBITMQ_PORT", "invalid", 1);
    setenv("REDIS_PORT", "not_a_number", 1);
    setenv("API_PORT", "abc", 1);

    auto& config = Config::getInstance();
    config.load();

    EXPECT_EQ(config.rabbitmq_port, 5672);
    EXPECT_EQ(config.redis_port, 6379);
    EXPECT_EQ(config.api_port, 8080);
}

TEST_F(ConfigTest, InvalidCacheSizeDefaultsTo10) {
    setenv("CACHE_SIZE", "not_a_number", 1);
    auto& config = Config::getInstance();
    config.load();
    EXPECT_EQ(config.cache_size, 10);

    setenv("CACHE_SIZE", "-5", 1);
    config.load();
    EXPECT_EQ(config.cache_size, 10);

    setenv("CACHE_SIZE", "0", 1);
    config.load();
    EXPECT_EQ(config.cache_size, 10);
}

TEST_F(ConfigTest, LargeCacheSize) {
    setenv("CACHE_SIZE", "10000", 1);
    auto& config = Config::getInstance();
    config.load();
    EXPECT_EQ(config.cache_size, 10000);
}

TEST_F(ConfigTest, EmptyPasswordIsValid) {
    setenv("RABBITMQ_PASSWORD", "", 1);
    setenv("REDIS_PASSWORD", "", 1);

    auto& config = Config::getInstance();
    config.load();

    EXPECT_EQ(config.rabbitmq_password, "");
    EXPECT_EQ(config.redis_password, "");
}

TEST_F(ConfigTest, WhitespaceInValues) {
    setenv("RABBITMQ_HOST", "  rabbitmq.example.com  ", 1);
    setenv("REDIS_HOST", "\tredis.example.com\n", 1);

    auto& config = Config::getInstance();
    config.load();

    EXPECT_EQ(config.rabbitmq_host, "  rabbitmq.example.com  ");
    EXPECT_EQ(config.redis_host, "\tredis.example.com\n");
}
