# syntax=docker/dockerfile:1
# check=skip=SecretsUsedInArgOrEnv
# Multi-stage build for optimized container size
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libpulse-dev \
    libevent-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

RUN git clone https://github.com/yhirose/cpp-httplib.git /opt/cpp-httplib && \
    git clone https://github.com/nlohmann/json.git /opt/nlohmann-json && \
    git clone https://github.com/gabime/spdlog.git /opt/spdlog && \
    git clone https://github.com/redis/hiredis.git /opt/hiredis && \
    git clone https://github.com/sewenew/redis-plus-plus.git /opt/redis-plus-plus && \
    git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git /opt/amqp-cpp

RUN cd /opt/hiredis && make && make install && \
    cd /opt/redis-plus-plus && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install && \
    cd /opt/amqp-cpp && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DAMQP-CPP_BUILD_SHARED=ON -DAMQP-CPP_LINUX_TCP=ON .. && \
    make && make install && \
    cd /opt/spdlog && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install

RUN mkdir -p /build/include && \
    cp -r /opt/cpp-httplib/httplib.h /build/include/ && \
    cp -r /opt/nlohmann-json/include/nlohmann /build/include/

COPY CMakeLists.txt /build/
COPY src/ /build/src/
COPY include/ /build/include/

RUN mkdir -p /build/build && cd /build/build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF .. && \
    make -j$(nproc) && \
    strip tts_playback_service

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libpulse0 \
    libevent-2.1-7 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/lib* /usr/local/lib/
COPY --from=builder /build/build/tts_playback_service /usr/local/bin/

RUN ldconfig

ENV RABBITMQ_HOST=rabbitmq \
    RABBITMQ_PORT=5672 \
    RABBITMQ_USER=guest \
    RABBITMQ_VHOST=/ \
    RABBITMQ_QUEUE=tts_playback_queue \
    REDIS_HOST=redis \
    REDIS_PORT=6379 \
    CACHE_SIZE=10 \
    API_HOST=0.0.0.0 \
    API_PORT=8080 \
    PULSEAUDIO_SINK="" \
    DOTMATRIX_ENABLED=0 \
    DOTMATRIX_QUEUE_DIR=/tmp/halo-dotmatrix/queue \
    DOTMATRIX_WAV_DIR=/tmp/halo-dotmatrix/wav \
    LOG_LEVEL=2

EXPOSE 8080

CMD ["/usr/local/bin/tts_playback_service"]
