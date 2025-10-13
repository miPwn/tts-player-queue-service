#include "api_server.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ApiServer::ApiServer(const std::string& host, int port)
    : host_(host), port_(port), server_(std::make_unique<httplib::Server>()) {
}

ApiServer::~ApiServer() {
    stop();
}

void ApiServer::setJobHandler(std::function<void(const TTSRequest&)> handler) {
    job_handler_ = handler;
}

void ApiServer::start() {
    server_->Post("/api/tts/play", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string text;
            std::vector<char> wav_data;
            bool has_text = false;
            bool has_wav = false;

            // Access multipart form data through params and files
            // Text field comes through params
            if (req.has_param("text")) {
                text = req.get_param_value("text");
                has_text = true;
            }

            // WAV file comes through files - iterate to find it
            for (size_t i = 0; i < req.files.size(); ++i) {
                const auto& file = req.files[i];
                if (file.name == "wav") {
                    wav_data.assign(file.content.begin(), file.content.end());
                    has_wav = true;
                    break;
                }
            }

            if (!has_text) {
                res.status = 400;
                res.set_content("{\"error\": \"Missing 'text' field\"}", "application/json");
                return;
            }

            if (!has_wav) {
                res.status = 400;
                res.set_content("{\"error\": \"Missing 'wav' file\"}", "application/json");
                return;
            }

            if (job_handler_) {
                TTSRequest tts_req;
                tts_req.text = text;
                tts_req.wav_data = wav_data;

                job_handler_(tts_req);

                json response;
                response["status"] = "queued";
                response["text"] = text.substr(0, 100);
                response["size"] = wav_data.size();

                res.set_content(response.dump(), "application/json");
                spdlog::info("API: Queued TTS job - text: {}, size: {} bytes",
                            text.substr(0, 50), wav_data.size());
            } else {
                res.status = 500;
                res.set_content("{\"error\": \"No job handler configured\"}", "application/json");
            }
        } catch (const std::exception& e) {
            spdlog::error("API error: {}", e.what());
            res.status = 500;
            json error_response;
            error_response["error"] = e.what();
            res.set_content(error_response.dump(), "application/json");
        }
    });

    server_->Get("/health", [](const httplib::Request&, httplib::Response& res) {
        json health;
        health["status"] = "healthy";
        health["service"] = "tts-playback-service";
        res.set_content(health.dump(), "application/json");
    });

    spdlog::info("Starting API server on {}:{}", host_, port_);
    server_->listen(host_.c_str(), port_);
}

void ApiServer::stop() {
    if (server_) {
        server_->stop();
        spdlog::info("API server stopped");
    }
}
