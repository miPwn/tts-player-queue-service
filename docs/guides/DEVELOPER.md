# Developer Guide

## TTS Playback Service

**Version:** 1.0.0  
**Last Updated:** October 13, 2025

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Environment Setup](#development-environment-setup)
3. [Project Structure](#project-structure)
4. [Building the Project](#building-the-project)
5. [Running Locally](#running-locally)
6. [Testing](#testing)
7. [Debugging](#debugging)
8. [Code Style & Standards](#code-style--standards)
9. [Contributing](#contributing)
10. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Prerequisites

- **Operating System**: Linux (Ubuntu 20.04+ recommended)
- **C++ Compiler**: GCC 11+ or Clang 14+
- **Build Tools**: CMake 3.20+, Make
- **Docker**: 20.10+ (for containerized development)
- **Git**: 2.30+

### Quick Start

```bash
# Clone repository
git clone https://github.com/your-org/tts-playback-service.git
cd tts-playback-service

# Start development environment
docker-compose up --build

# Service available at http://localhost:8080
```

---

## Development Environment Setup

### Option 1: Docker Development (Recommended)

**Pros**: Consistent environment, all dependencies included  
**Cons**: Slower compilation, requires Docker

```bash
# Start full stack
docker-compose up -d

# View logs
docker-compose logs -f tts-playback

# Stop services
docker-compose down
```

### Option 2: Native Development

**Pros**: Faster compilation, direct debugging  
**Cons**: Manual dependency management

#### Install System Dependencies

**Ubuntu/Debian**:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libpulse-dev \
    libevent-dev \
    libssl-dev
```

**Arch Linux**:
```bash
sudo pacman -S base-devel cmake git pkgconf \
    libpulse libevent openssl
```

#### Install C++ Libraries

```bash
# Clone and build hiredis
git clone https://github.com/redis/hiredis.git
cd hiredis && make && sudo make install

# Clone and build redis-plus-plus
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make && sudo make install

# Clone and build AMQP-CPP
git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
cd AMQP-CPP && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DAMQP-CPP_BUILD_SHARED=ON \
      -DAMQP-CPP_LINUX_TCP=ON ..
make && sudo make install

# Clone and build spdlog
git clone https://github.com/gabime/spdlog.git
cd spdlog && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make && sudo make install

# Update library cache
sudo ldconfig
```

#### Install Header-Only Libraries

```bash
# cpp-httplib
wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h \
    -O include/httplib.h

# nlohmann/json
mkdir -p include/nlohmann
wget https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
    -O include/nlohmann/json.hpp
```

#### Start External Services

```bash
# Start Redis
docker run -d -p 6379:6379 --name redis redis:7-alpine

# Start RabbitMQ
docker run -d -p 5672:5672 -p 15672:15672 \
    --name rabbitmq rabbitmq:3.12-management-alpine
```

---

## Project Structure

```
tts-playback-service/
├── include/                    # Header files
│   ├── config.h               # Configuration singleton
│   ├── redis_cache.h          # Redis cache manager
│   ├── rabbitmq_client.h      # RabbitMQ client
│   ├── audio_player.h         # Audio playback
│   └── api_server.h           # REST API server
│
├── src/                       # Source files
│   ├── main.cpp               # Entry point
│   ├── redis_cache.cpp        # Cache implementation
│   ├── rabbitmq_client.cpp    # Queue implementation
│   ├── audio_player.cpp       # Playback implementation
│   └── api_server.cpp         # API implementation
│
├── docs/                      # Documentation
│   ├── technical/             # Technical docs
│   │   ├── DESIGN.md
│   │   └── ARCHITECTURE.md
│   ├── guides/                # User guides
│   │   ├── DEVELOPER.md
│   │   └── INFRASTRUCTURE.md
│   └── api/                   # API docs
│       ├── openapi.yaml
│       └── API.md
│
├── CMakeLists.txt             # Build configuration
├── Dockerfile                 # Container build
├── docker-compose.yml         # Stack deployment
├── .env.example               # Config template
├── README.md                  # Main documentation
└── PROJECT_SUMMARY.md         # Quick reference

```

### Key Files

| File | Purpose | When to Modify |
|------|---------|---------------|
| `include/*.h` | Interface definitions | Adding new features |
| `src/*.cpp` | Implementation | Bug fixes, enhancements |
| `CMakeLists.txt` | Build config | Adding dependencies |
| `Dockerfile` | Container build | Changing base image |
| `docker-compose.yml` | Stack config | Changing services |

---

## Building the Project

### CMake Build (Debug)

```bash
# Create build directory
mkdir -p build && cd build

# Configure with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build
make -j$(nproc)

# Binary location: build/tts_playback_service
```

### CMake Build (Release)

```bash
# Create build directory
mkdir -p build && cd build

# Configure with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with optimizations
make -j$(nproc)

# Strip binary (optional)
strip tts_playback_service
```

### Build Options

```bash
# Verbose build
make VERBOSE=1

# Clean build
make clean

# Rebuild from scratch
rm -rf build && mkdir build && cd build && cmake .. && make
```

### Compilation Flags

**Debug Build**:
```cmake
-g -O0 -Wall -Wextra -Wpedantic
```

**Release Build**:
```cmake
-O3 -march=native -DNDEBUG -flto -Wall -Wextra
```

---

## Running Locally

### Set Environment Variables

```bash
# Copy template
cp .env.example .env

# Edit configuration
nano .env

# Load environment
export $(cat .env | xargs)
```

### Run Binary

```bash
# With external services running
./build/tts_playback_service

# Expected output:
# [info] === TTS Playback Service Starting ===
# [info] RabbitMQ: localhost:5672
# [info] Redis: localhost:6379
# [info] API: 0.0.0.0:8080
# [info] Cache Size: 10
# [info] Redis cache initialized with max size: 10
# [info] RabbitMQ connected to localhost:5672
# [info] Audio player initialized with sink: default
# [info] Starting API server on 0.0.0.0:8080
# [info] === TTS Playback Service Running ===
```

### Test API

```bash
# Generate test WAV file
sox -n -r 44100 -c 2 test.wav synth 1 sine 440

# Submit playback job
curl -X POST http://localhost:8080/api/tts/play \
  -F "text=Test audio playback" \
  -F "wav=@test.wav"

# Check health
curl http://localhost:8080/health
```

---

## Testing

### Manual Testing

#### Test Cache Hit
```bash
# First request (cache miss)
curl -X POST http://localhost:8080/api/tts/play \
  -F "text=Cached audio" \
  -F "wav=@test.wav"

# Second request (cache hit)
curl -X POST http://localhost:8080/api/tts/play \
  -F "text=Cached audio" \
  -F "wav=@test.wav"

# Check Redis
docker exec -it redis redis-cli
> GET tts:wav:Cached audio
> ZRANGE tts:cache:lru 0 -1 WITHSCORES
```

#### Test Queue Processing
```bash
# Submit multiple jobs rapidly
for i in {1..5}; do
  curl -X POST http://localhost:8080/api/tts/play \
    -F "text=Job $i" \
    -F "wav=@test.wav" &
done
wait

# Check RabbitMQ queue
curl -u guest:guest http://localhost:15672/api/queues/%2F/tts_playback_queue
```

### Unit Testing (Future)

```cpp
// Example unit test structure
#include <gtest/gtest.h>
#include "redis_cache.h"

TEST(RedisCacheTest, LRUEviction) {
    RedisCache cache("localhost", 6379, "", 2);
    
    std::vector<char> wav1 = {0x01, 0x02};
    std::vector<char> wav2 = {0x03, 0x04};
    std::vector<char> wav3 = {0x05, 0x06};
    
    cache.put("text1", wav1);
    cache.put("text2", wav2);
    cache.put("text3", wav3);  // Should evict text1
    
    std::vector<char> result;
    EXPECT_FALSE(cache.get("text1", result));  // Evicted
    EXPECT_TRUE(cache.get("text2", result));   // Present
    EXPECT_TRUE(cache.get("text3", result));   // Present
}
```

### Integration Testing

```bash
# Start test environment
docker-compose -f docker-compose.test.yml up -d

# Run integration tests
./scripts/integration_test.sh

# Cleanup
docker-compose -f docker-compose.test.yml down
```

---

## Debugging

### GDB Debugging

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug .. && make

# Run with GDB
gdb ./tts_playback_service

# GDB commands:
(gdb) break main
(gdb) run
(gdb) next
(gdb) print variable_name
(gdb) backtrace
(gdb) continue
```

### Valgrind Memory Analysis

```bash
# Check for memory leaks
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./tts_playback_service

# Check for thread issues
valgrind --tool=helgrind ./tts_playback_service
```

### Logging Levels

```bash
# Trace logging (most verbose)
export LOG_LEVEL=0

# Debug logging
export LOG_LEVEL=1

# Info logging (default)
export LOG_LEVEL=2

# Warning only
export LOG_LEVEL=3

# Error only
export LOG_LEVEL=4
```

### Common Debug Scenarios

#### Issue: RabbitMQ Connection Fails

```bash
# Check RabbitMQ status
docker ps | grep rabbitmq

# Check connection
telnet localhost 5672

# View RabbitMQ logs
docker logs rabbitmq
```

#### Issue: Redis Connection Fails

```bash
# Check Redis status
docker ps | grep redis

# Test connection
redis-cli ping

# View Redis logs
docker logs redis
```

#### Issue: Audio Playback Fails

```bash
# Check PulseAudio
pulseaudio --check
pactl info

# List audio sinks
pactl list sinks short

# Test audio
speaker-test -t sine -f 440 -c 2
```

---

## Code Style & Standards

### C++ Style Guide

**Naming Conventions**:
```cpp
// Classes: PascalCase
class RedisCache { };

// Functions: camelCase
void publishJob() { }

// Variables: snake_case
std::string rabbitmq_host;

// Constants: UPPER_SNAKE_CASE
const int MAX_CACHE_SIZE = 1000;

// Private members: trailing underscore
class Foo {
    int value_;
};
```

**Formatting**:
```cpp
// Indentation: 4 spaces
// Braces: Same line for functions, new line for classes
// Max line length: 100 characters

class Example {
public:
    void doSomething() {
        if (condition) {
            // code
        }
    }

private:
    int value_;
};
```

**Best Practices**:
- Use RAII for resource management
- Prefer `std::unique_ptr` over raw pointers
- Use `const` where possible
- Pass large objects by const reference
- Use `std::move` for transfers
- Avoid `using namespace std`

### Code Review Checklist

- [ ] Code compiles without warnings
- [ ] No memory leaks (Valgrind clean)
- [ ] Thread-safe where needed
- [ ] Error handling implemented
- [ ] Logging added for key operations
- [ ] Documentation updated
- [ ] Tests added/updated

---

## Contributing

### Workflow

1. **Fork the repository**
2. **Create feature branch**: `git checkout -b feature/my-feature`
3. **Make changes**: Follow code style guide
4. **Test changes**: Run tests and manual verification
5. **Commit**: Use descriptive messages
6. **Push**: `git push origin feature/my-feature`
7. **Create Pull Request**: Include description and tests

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Maintenance

**Example**:
```
feat(cache): add TTL support for Redis cache

Implement time-to-live expiration for cached WAV files.
Configurable via CACHE_TTL environment variable.

Closes #123
```

---

## Troubleshooting

### Build Issues

#### CMake can't find dependencies

```bash
# Update pkg-config path
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# Rebuild with verbose
cmake -DCMAKE_BUILD_TYPE=Debug .. && make VERBOSE=1
```

#### Linking errors

```bash
# Update library cache
sudo ldconfig

# Check library paths
ldconfig -p | grep <library>
```

### Runtime Issues

#### Segmentation fault

```bash
# Run with core dump
ulimit -c unlimited
./tts_playback_service
# If crash: gdb ./tts_playback_service core
```

#### High memory usage

```bash
# Check cache size
redis-cli INFO memory

# Monitor process
top -p $(pgrep tts_playback)
```

#### Audio stuttering

```bash
# Check CPU usage
mpstat 1

# Increase PulseAudio buffer
export PULSE_LATENCY_MSEC=50
```

---

## Additional Resources

- [Technical Design](../technical/DESIGN.md)
- [Architecture Guide](../technical/ARCHITECTURE.md)
- [API Documentation](../api/API.md)
- [Infrastructure Guide](INFRASTRUCTURE.md)

---

## Getting Help

- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions
- **Documentation**: `/docs` directory
- **Examples**: `/examples` directory

---

**Happy Coding! 🚀**
