#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "api_server.h"
#include <thread>
#include <chrono>
#include <httplib.h>

class ApiServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        host = "127.0.0.1";
        port = 18080;
        job_count = 0;
    }

    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::string host;
    int port;
    std::atomic<int> job_count;

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

TEST_F(ApiServerTest, HealthEndpoint) {
    auto callback = [&](const TTSRequest& req) {
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);
    auto res = client.Get("/health");

    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_THAT(res->body, testing::HasSubstr("healthy"));
    EXPECT_THAT(res->body, testing::HasSubstr("tts-playback-service"));

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, PlayEndpointWithValidData) {
    TTSRequest received_request;
    auto callback = [&](const TTSRequest& req) {
        received_request = req;
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);
    httplib::MultipartFormDataItems items = {
        {"text", "Hello world", "", ""},
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };

    auto res = client.Post("/api/tts/play", items);

    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_THAT(res->body, testing::HasSubstr("queued"));
    EXPECT_THAT(res->body, testing::HasSubstr("Hello world"));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(1, job_count.load());
    EXPECT_EQ("Hello world", received_request.text);
    EXPECT_FALSE(received_request.wav_data.empty());

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, PlayEndpointMissingTextField) {
    auto callback = [&](const TTSRequest& req) {
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);
    httplib::MultipartFormDataItems items = {
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };

    auto res = client.Post("/api/tts/play", items);

    ASSERT_TRUE(res);
    EXPECT_EQ(400, res->status);
    EXPECT_THAT(res->body, testing::HasSubstr("error"));
    EXPECT_THAT(res->body, testing::HasSubstr("text"));

    EXPECT_EQ(0, job_count.load());

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, PlayEndpointMissingWavFile) {
    auto callback = [&](const TTSRequest& req) {
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);
    httplib::MultipartFormDataItems items = {
        {"text", "Hello world", "", ""}
    };

    auto res = client.Post("/api/tts/play", items);

    ASSERT_TRUE(res);
    EXPECT_EQ(400, res->status);
    EXPECT_THAT(res->body, testing::HasSubstr("error"));
    EXPECT_THAT(res->body, testing::HasSubstr("wav"));

    EXPECT_EQ(0, job_count.load());

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, PlayEndpointWrongContentType) {
    auto callback = [&](const TTSRequest& req) {
        job_count++;
    };

    ApiServer server(host, port, callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);
    auto res = client.Post("/api/tts/play", "{\"text\":\"test\"}", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(400, res->status);
    EXPECT_THAT(res->body, testing::HasSubstr("multipart/form-data"));

    EXPECT_EQ(0, job_count.load());

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, MultipleRequests) {
    std::vector<TTSRequest> received_requests;
    std::mutex requests_mutex;

    auto callback = [&](const TTSRequest& req) {
        std::lock_guard<std::mutex> lock(requests_mutex);
        received_requests.push_back(req);
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);

    for (int i = 0; i < 5; ++i) {
        httplib::MultipartFormDataItems items = {
            {"text", "Request " + std::to_string(i), "", ""},
            {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
        };

        auto res = client.Post("/api/tts/play", items);
        ASSERT_TRUE(res);
        EXPECT_EQ(200, res->status);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(5, job_count.load());
    EXPECT_EQ(5, received_requests.size());

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, LongText) {
    TTSRequest received_request;
    auto callback = [&](const TTSRequest& req) {
        received_request = req;
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string long_text(5000, 'A');

    httplib::Client client(host, port);
    httplib::MultipartFormDataItems items = {
        {"text", long_text, "", ""},
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };

    auto res = client.Post("/api/tts/play", items);

    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(1, job_count.load());
    EXPECT_EQ(long_text, received_request.text);

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, SpecialCharactersInText) {
    TTSRequest received_request;
    auto callback = [&](const TTSRequest& req) {
        received_request = req;
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string special_text = "Hello\nWorld\tWith\"Quotes\"";

    httplib::Client client(host, port);
    httplib::MultipartFormDataItems items = {
        {"text", special_text, "", ""},
        {"wav", std::string(createMinimalWav().begin(), createMinimalWav().end()), "test.wav", "audio/wav"}
    };

    auto res = client.Post("/api/tts/play", items);

    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(special_text, received_request.text);

    server.stop();
    server_thread.join();
}

TEST_F(ApiServerTest, NotFoundEndpoint) {
    auto callback = [&](const TTSRequest& req) {
        job_count++;
    };

    ApiServer server(host, port);
    server.setJobHandler(callback);
    std::thread server_thread([&]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client client(host, port);
    auto res = client.Get("/nonexistent");

    ASSERT_TRUE(res);
    EXPECT_EQ(404, res->status);

    server.stop();
    server_thread.join();
}
