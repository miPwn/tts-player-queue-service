#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "rabbitmq_client.h"
#include <thread>
#include <chrono>
#include <atomic>

class RabbitMQClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        host = "localhost";
        port = 5672;
        user = "guest";
        password = "guest";
        vhost = "/";
        queue = "test_queue";
    }
    
    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::string host;
    int port;
    std::string user;
    std::string password;
    std::string vhost;
    std::string queue;
};

TEST_F(RabbitMQClientTest, BasicPublishAndConsume) {
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Test message";
    job.wav_data = {'T', 'E', 'S', 'T'};
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.text, received_job.text);
    EXPECT_EQ(job.wav_data, received_job.wav_data);
}

TEST_F(RabbitMQClientTest, PublishMultipleMessages) {
    std::atomic<int> message_count{0};
    std::vector<PlaybackJob> received_jobs;
    std::mutex jobs_mutex;
    
    auto callback = [&](const PlaybackJob& job) {
        std::lock_guard<std::mutex> lock(jobs_mutex);
        received_jobs.push_back(job);
        message_count++;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    for (int i = 0; i < 5; ++i) {
        PlaybackJob job;
        job.text = "Message " + std::to_string(i);
        job.wav_data = {static_cast<char>('A' + i)};
        client.publishJob(job);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(5, message_count.load());
    EXPECT_EQ(5, received_jobs.size());
}

TEST_F(RabbitMQClientTest, EmptyWavData) {
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Empty audio";
    job.wav_data = {};
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.text, received_job.text);
    EXPECT_TRUE(received_job.wav_data.empty());
}

TEST_F(RabbitMQClientTest, LargeWavData) {
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Large audio";
    job.wav_data = std::vector<char>(1024 * 100, 'X');
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.text, received_job.text);
    EXPECT_EQ(job.wav_data.size(), received_job.wav_data.size());
}

TEST_F(RabbitMQClientTest, BinaryWavData) {
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Binary data";
    job.wav_data.clear();
    for (int i = 0; i < 256; ++i) {
        job.wav_data.push_back(static_cast<char>(i));
    }
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.wav_data, received_job.wav_data);
}

TEST_F(RabbitMQClientTest, SpecialCharactersInText) {
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Special: \n\t\"quotes\" and 'apostrophes' with \\backslash";
    job.wav_data = {'T', 'E', 'S', 'T'};
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.text, received_job.text);
}

TEST_F(RabbitMQClientTest, ThreadSafePublish) {
    std::atomic<int> message_count{0};
    
    auto callback = [&](const PlaybackJob& job) {
        message_count++;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto worker = [&client](int thread_id) {
        for (int i = 0; i < 5; ++i) {
            PlaybackJob job;
            job.text = "Thread_" + std::to_string(thread_id) + "_Msg_" + std::to_string(i);
            job.wav_data = {static_cast<char>('A' + thread_id)};
            client.publishJob(job);
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(15, message_count.load());
}

TEST_F(RabbitMQClientTest, Base64Encoding) {
    std::atomic<bool> message_received{false};
    PlaybackJob received_job;
    
    auto callback = [&](const PlaybackJob& job) {
        received_job = job;
        message_received = true;
    };
    
    RabbitMQClient client(host, port, user, password, vhost, queue, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    PlaybackJob job;
    job.text = "Base64 test";
    job.wav_data = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    
    client.publishJob(job);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(message_received);
    EXPECT_EQ(job.wav_data, received_job.wav_data);
}
