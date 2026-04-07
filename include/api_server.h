#pragma once

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "httplib.h"

struct TTSRequest {
    std::string text;
    std::vector<char> wav_data;
};

class ApiServer {
public:
    ApiServer(const std::string& host, int port);
    ~ApiServer();

    void setJobHandler(std::function<void(const TTSRequest&)> handler);
    void setHealthHandler(std::function<nlohmann::json()> handler);
    void start();
    void stop();

private:
    std::string host_;
    int port_;
    std::unique_ptr<httplib::Server> server_;
    std::function<void(const TTSRequest&)> job_handler_;
    std::function<nlohmann::json()> health_handler_;
};
