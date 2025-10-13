#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "  TTS Playback Service - Test Runner"
echo "========================================"
echo ""

check_service() {
    local service=$1
    local host=$2
    local port=$3
    
    if nc -z "$host" "$port" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $service is running on $host:$port"
        return 0
    else
        echo -e "${RED}✗${NC} $service is NOT running on $host:$port"
        return 1
    fi
}

echo "Checking external services..."
echo ""

REDIS_OK=true
RABBITMQ_OK=true

if ! check_service "Redis" "localhost" "6379"; then
    REDIS_OK=false
    echo "  Start with: docker run -d -p 6379:6379 redis:7-alpine"
fi

if ! check_service "RabbitMQ" "localhost" "5672"; then
    RABBITMQ_OK=false
    echo "  Start with: docker run -d -p 5672:5672 -p 15672:15672 rabbitmq:3.12-management-alpine"
fi

echo ""

if [ "$REDIS_OK" = false ] || [ "$RABBITMQ_OK" = false ]; then
    echo -e "${YELLOW}Warning: Some services are not running${NC}"
    echo "Some tests may fail. Continue anyway? [y/N]"
    read -r response
    if [[ ! "$response" =~ ^[Yy]$ ]]; then
        echo "Aborted."
        exit 1
    fi
fi

BUILD_DIR="${BUILD_DIR:-build}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Creating..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

if [ ! -f "CMakeCache.txt" ]; then
    echo "Configuring build..."
    cmake -DCMAKE_BUILD_TYPE=Debug ..
fi

echo "Building tests..."
make -j$(nproc)

echo ""
echo "========================================"
echo "  Running Tests"
echo "========================================"
echo ""

if [ -n "$1" ]; then
    echo "Running specific test: $1"
    "./$1" "${@:2}"
else
    echo "Running all tests with CTest..."
    ctest --output-on-failure "$@"
fi

echo ""
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo ""

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
