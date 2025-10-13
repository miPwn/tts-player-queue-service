#include "config.h"
#include "redis_cache.h"
#include "rabbitmq_client.h"
#include "audio_player.h"
#include "api_server.h"
#include <spdlog/spdlog.h>
#include <signal.h>
#include <thread>
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signum) {
    spdlog::info("Received signal {}, shutting down...", signum);
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    auto& config = Config::getInstance();
    config.load();

    switch (config.log_level) {
        case 0: spdlog::set_level(spdlog::level::trace); break;
        case 1: spdlog::set_level(spdlog::level::debug); break;
        case 2: spdlog::set_level(spdlog::level::info); break;
        case 3: spdlog::set_level(spdlog::level::warn); break;
        case 4: spdlog::set_level(spdlog::level::err); break;
        default: spdlog::set_level(spdlog::level::info); break;
    }

    spdlog::info("=== TTS Playback Service Starting ===");
    spdlog::info("RabbitMQ: {}:{}", config.rabbitmq_host, config.rabbitmq_port);
    spdlog::info("Redis: {}:{}", config.redis_host, config.redis_port);
    spdlog::info("API: {}:{}", config.api_host, config.api_port);
    spdlog::info("Cache Size: {}", config.cache_size);

    try {
        auto cache = std::make_shared<RedisCache>(
            config.redis_host,
            config.redis_port,
            config.redis_password,
            config.cache_size
        );

        auto rabbitmq = std::make_shared<RabbitMQClient>(
            config.rabbitmq_host,
            config.rabbitmq_port,
            config.rabbitmq_user,
            config.rabbitmq_password,
            config.rabbitmq_vhost
        );

        auto player = std::make_shared<AudioPlayer>(config.pulseaudio_sink);

        rabbitmq->connect();

        rabbitmq->consumeJobs(config.rabbitmq_queue, [cache, player](const PlaybackJob& job) {
            spdlog::info("Processing playback job: {}", job.text.substr(0, 50));
            
            try {
                player->play(job.wav_data);
                spdlog::info("Successfully played audio for: {}", job.text.substr(0, 50));
            } catch (const std::exception& e) {
                spdlog::error("Playback error: {}", e.what());
            }
        });

        std::thread rabbitmq_thread([rabbitmq]() {
            rabbitmq->startEventLoop();
        });

        ApiServer api_server(config.api_host, config.api_port);
        
        api_server.setJobHandler([cache, rabbitmq, &config](const TTSRequest& req) {
            std::vector<char> cached_wav;
            
            if (cache->get(req.text, cached_wav)) {
                spdlog::info("Using cached WAV for: {}", req.text.substr(0, 50));
                
                PlaybackJob job;
                job.text = req.text;
                job.wav_data = cached_wav;
                rabbitmq->publishJob(job, config.rabbitmq_queue);
            } else {
                spdlog::info("Caching new WAV for: {}", req.text.substr(0, 50));
                cache->put(req.text, req.wav_data);
                
                PlaybackJob job;
                job.text = req.text;
                job.wav_data = req.wav_data;
                rabbitmq->publishJob(job, config.rabbitmq_queue);
            }
        });

        std::thread api_thread([&api_server]() {
            api_server.start();
        });

        spdlog::info("=== TTS Playback Service Running ===");

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        spdlog::info("Stopping services...");
        api_server.stop();
        rabbitmq->stop();

        if (api_thread.joinable()) api_thread.join();
        if (rabbitmq_thread.joinable()) rabbitmq_thread.join();

        spdlog::info("=== TTS Playback Service Stopped ===");
        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }
}
