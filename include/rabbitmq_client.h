#pragma once

#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <event2/event.h>

struct PlaybackJob {
    std::string text;
    std::vector<char> wav_data;
};

class RabbitMQClient {
public:
    RabbitMQClient(const std::string& host, int port, const std::string& user, 
                   const std::string& password, const std::string& vhost);
    ~RabbitMQClient();

    void connect();
    void publishJob(const PlaybackJob& job, const std::string& queue);
    void consumeJobs(const std::string& queue, std::function<void(const PlaybackJob&)> callback);
    void startEventLoop();
    void stop();

private:
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string vhost_;
    
    struct event_base* event_base_;
    std::unique_ptr<AMQP::LibEventHandler> handler_;
    std::unique_ptr<AMQP::TcpConnection> connection_;
    std::unique_ptr<AMQP::TcpChannel> channel_;
    
    bool running_;
    
    struct PublishTask {
        PlaybackJob job;
        std::string queue;
    };
    
    std::queue<PublishTask> publish_queue_;
    std::mutex queue_mutex_;
    struct event* dispatch_event_;
    
    void processPublishQueue();
    static void dispatchCallback(evutil_socket_t fd, short what, void* arg);
    void publishJobInternal(const PlaybackJob& job, const std::string& queue);
    
    std::string base64Encode(const std::vector<char>& data);
    std::vector<char> base64Decode(const std::string& encoded);
};
