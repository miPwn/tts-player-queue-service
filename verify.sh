#!/bin/bash

echo "=========================================="
echo "TTS Playback Service - Project Verification"
echo "=========================================="
echo ""

echo "✓ Checking project structure..."
echo ""

check_file() {
    if [ -f "$1" ]; then
        echo "  ✓ $1"
    else
        echo "  ✗ $1 (missing)"
        exit 1
    fi
}

check_dir() {
    if [ -d "$1" ]; then
        echo "  ✓ $1/"
    else
        echo "  ✗ $1/ (missing)"
        exit 1
    fi
}

echo "Core Application Files:"
check_file "CMakeLists.txt"
check_file "Dockerfile"
check_file "docker-compose.yml"
check_file "README.md"
check_file ".env.example"
check_file ".gitignore"
echo ""

echo "Source Files:"
check_file "src/main.cpp"
check_file "src/redis_cache.cpp"
check_file "src/rabbitmq_client.cpp"
check_file "src/audio_player.cpp"
check_file "src/api_server.cpp"
echo ""

echo "Header Files:"
check_file "include/config.h"
check_file "include/redis_cache.h"
check_file "include/rabbitmq_client.h"
check_file "include/audio_player.h"
check_file "include/api_server.h"
echo ""

echo "✓ All files present!"
echo ""

echo "=========================================="
echo "Architecture Overview"
echo "=========================================="
echo ""
echo "Components:"
echo "  • REST API (cpp-httplib) - Port 8080"
echo "  • RabbitMQ Queue - Async job processing"
echo "  • Redis Cache - LRU caching (configurable size)"
echo "  • PulseAudio - Synchronous playback"
echo ""

echo "Configuration (via environment variables):"
echo "  • RABBITMQ_HOST, RABBITMQ_PORT, RABBITMQ_USER, RABBITMQ_PASSWORD"
echo "  • REDIS_HOST, REDIS_PORT, CACHE_SIZE"
echo "  • API_HOST, API_PORT"
echo "  • PULSEAUDIO_SINK"
echo "  • LOG_LEVEL"
echo ""

echo "=========================================="
echo "Deployment Instructions"
echo "=========================================="
echo ""
echo "This service requires Docker/Kubernetes infrastructure:"
echo ""
echo "1. Docker Compose (Local Development):"
echo "   docker-compose up --build"
echo ""
echo "2. Docker Build (Manual):"
echo "   docker build -t tts-playback-service ."
echo "   docker run -p 8080:8080 tts-playback-service"
echo ""
echo "3. Kubernetes Deployment:"
echo "   See README.md for K8s manifest examples"
echo ""

echo "=========================================="
echo "API Usage Example"
echo "=========================================="
echo ""
echo "Submit TTS job:"
echo "  curl -X POST http://localhost:8080/api/tts/play \\"
echo "    -F \"text=Hello world\" \\"
echo "    -F \"wav=@audio.wav\""
echo ""
echo "Health check:"
echo "  curl http://localhost:8080/health"
echo ""

echo "=========================================="
echo "Code Statistics"
echo "=========================================="
echo ""
echo "Lines of code:"
wc -l src/*.cpp include/*.h | tail -1
echo ""

echo "=========================================="
echo "✓ Project verification complete!"
echo "=========================================="
echo ""
echo "NOTE: This service requires external infrastructure"
echo "(RabbitMQ, Redis, PulseAudio) and must be deployed"
echo "using Docker or Kubernetes. See README.md for details."
echo ""
