#pragma once

#include <string>
#include <cstdlib>
#include <stdexcept>

class Config {
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    std::string rabbitmq_host;
    int rabbitmq_port;
    std::string rabbitmq_user;
    std::string rabbitmq_password;
    std::string rabbitmq_vhost;
    std::string rabbitmq_queue;

    std::string redis_host;
    int redis_port;
    std::string redis_password;
    int cache_size;

    std::string api_host;
    int api_port;

    std::string pulseaudio_sink;

    bool dotmatrix_enabled;
    std::string dotmatrix_queue_dir;
    std::string dotmatrix_wav_dir;

    int log_level;

    void load() {
        rabbitmq_host = getEnv("RABBITMQ_HOST", "localhost");
        rabbitmq_port = getEnvInt("RABBITMQ_PORT", 5672);
        rabbitmq_user = getEnv("RABBITMQ_USER", "guest");
        rabbitmq_password = getEnv("RABBITMQ_PASSWORD", "guest");
        rabbitmq_vhost = getEnv("RABBITMQ_VHOST", "/");
        rabbitmq_queue = getEnv("RABBITMQ_QUEUE", "tts_playback_queue");

        redis_host = getEnv("REDIS_HOST", "localhost");
        redis_port = getEnvInt("REDIS_PORT", 6379);
        redis_password = getEnv("REDIS_PASSWORD", "");
        cache_size = getEnvInt("CACHE_SIZE", 10);

        api_host = getEnv("API_HOST", "0.0.0.0");
        api_port = getEnvInt("API_PORT", 8080);

        pulseaudio_sink = getEnv("PULSEAUDIO_SINK", "");

        dotmatrix_enabled = getEnvInt("DOTMATRIX_ENABLED", 0) != 0;
        dotmatrix_queue_dir = getEnv("DOTMATRIX_QUEUE_DIR", "/tmp/halo-dotmatrix/queue");
        dotmatrix_wav_dir = getEnv("DOTMATRIX_WAV_DIR", "/tmp/halo-dotmatrix/wav");

        log_level = getEnvInt("LOG_LEVEL", 2);
    }

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string getEnv(const char* name, const std::string& default_value) {
        const char* value = std::getenv(name);
        return value ? std::string(value) : default_value;
    }

    int getEnvInt(const char* name, int default_value) {
        const char* value = std::getenv(name);
        if (!value) return default_value;
        try {
            return std::stoi(value);
        } catch (...) {
            return default_value;
        }
    }
};
