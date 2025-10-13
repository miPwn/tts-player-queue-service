#include "rabbitmq_client.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <array>

using json = nlohmann::json;

static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

RabbitMQClient::RabbitMQClient(const std::string& host, int port, const std::string& user,
                               const std::string& password, const std::string& vhost)
    : host_(host), port_(port), user_(user), password_(password), vhost_(vhost), 
      event_base_(nullptr), dispatch_event_(nullptr), running_(false) {
}

RabbitMQClient::~RabbitMQClient() {
    stop();
}

std::string RabbitMQClient::base64Encode(const std::vector<char>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = data.size();
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(data.data());

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::vector<char> RabbitMQClient::base64Decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<char> ret;

    while (in_len-- && (encoded_string[in_] != '=')) {
        if (!isalnum(encoded_string[in_]) && encoded_string[in_] != '+' && encoded_string[in_] != '/') {
            in_++;
            continue;
        }
        
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }

    return ret;
}

void RabbitMQClient::connect() {
    event_base_ = event_base_new();
    if (!event_base_) {
        throw std::runtime_error("Failed to create event base");
    }

    handler_ = std::make_unique<AMQP::LibEventHandler>(event_base_);
    
    AMQP::Address address(host_, port_, AMQP::Login(user_, password_), vhost_);
    connection_ = std::make_unique<AMQP::TcpConnection>(handler_.get(), address);
    channel_ = std::make_unique<AMQP::TcpChannel>(connection_.get());

    channel_->onError([](const char* message) {
        spdlog::error("RabbitMQ channel error: {}", message);
    });

    dispatch_event_ = event_new(event_base_, -1, EV_PERSIST, dispatchCallback, this);
    struct timeval tv = {0, 10000};
    event_add(dispatch_event_, &tv);

    spdlog::info("RabbitMQ connected to {}:{}", host_, port_);
}

void RabbitMQClient::dispatchCallback(evutil_socket_t fd, short what, void* arg) {
    auto* client = static_cast<RabbitMQClient*>(arg);
    client->processPublishQueue();
}

void RabbitMQClient::processPublishQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    while (!publish_queue_.empty()) {
        auto task = publish_queue_.front();
        publish_queue_.pop();
        
        publishJobInternal(task.job, task.queue);
    }
}

void RabbitMQClient::publishJob(const PlaybackJob& job, const std::string& queue) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    publish_queue_.push({job, queue});
    spdlog::debug("Queued job for publishing: {}", job.text.substr(0, 50));
}

void RabbitMQClient::publishJobInternal(const PlaybackJob& job, const std::string& queue) {
    if (!channel_) {
        spdlog::error("RabbitMQ not connected, dropping job");
        return;
    }

    json j;
    j["text"] = job.text;
    j["wav_data"] = base64Encode(job.wav_data);
    
    std::string message = j.dump();
    
    channel_->declareQueue(queue, AMQP::durable);
    channel_->publish("", queue, message);
    
    spdlog::debug("Published job to queue '{}': {}", queue, job.text.substr(0, 50));
}

void RabbitMQClient::consumeJobs(const std::string& queue, 
                                  std::function<void(const PlaybackJob&)> callback) {
    if (!channel_) {
        throw std::runtime_error("RabbitMQ not connected");
    }

    channel_->declareQueue(queue, AMQP::durable);
    channel_->consume(queue)
        .onReceived([callback, this](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
            try {
                std::string body(message.body(), message.bodySize());
                json j = json::parse(body);
                
                PlaybackJob job;
                job.text = j["text"].get<std::string>();
                std::string wav_b64 = j["wav_data"].get<std::string>();
                job.wav_data = base64Decode(wav_b64);
                
                callback(job);
                
                channel_->ack(deliveryTag);
                spdlog::debug("Consumed and acknowledged job: {}", job.text.substr(0, 50));
            } catch (const std::exception& e) {
                spdlog::error("Error processing message: {}", e.what());
                channel_->reject(deliveryTag);
            }
        });

    spdlog::info("Consuming jobs from queue '{}'", queue);
}

void RabbitMQClient::startEventLoop() {
    running_ = true;
    spdlog::info("Starting RabbitMQ event loop");
    event_base_dispatch(event_base_);
}

void RabbitMQClient::stop() {
    if (running_) {
        running_ = false;
        if (dispatch_event_) {
            event_del(dispatch_event_);
            event_free(dispatch_event_);
            dispatch_event_ = nullptr;
        }
        if (event_base_) {
            event_base_loopbreak(event_base_);
            event_base_free(event_base_);
            event_base_ = nullptr;
        }
        channel_.reset();
        connection_.reset();
        handler_.reset();
        spdlog::info("RabbitMQ client stopped");
    }
}
