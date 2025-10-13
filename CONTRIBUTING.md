# Contributing to TTS Playback Service

Thank you for your interest in contributing to the TTS Playback Service! This document provides guidelines and instructions for contributing.

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [Development Workflow](#development-workflow)
4. [Coding Standards](#coding-standards)
5. [Testing Requirements](#testing-requirements)
6. [Pull Request Process](#pull-request-process)
7. [CI/CD Pipeline](#cicd-pipeline)

## Code of Conduct

This project follows a professional code of conduct. Please be respectful and constructive in all interactions.

## Getting Started

### Prerequisites

- C++17 compiler (GCC 11+ or Clang 14+)
- CMake 3.20+
- Docker and Docker Compose
- Git

### Fork and Clone

```bash
# Fork the repository on GitHub
# Then clone your fork
git clone https://github.com/YOUR_USERNAME/tts-player-queue-service.git
cd tts-player-queue-service

# Add upstream remote
git remote add upstream https://github.com/miPwn/tts-player-queue-service.git
```

### Set Up Development Environment

```bash
# Start external services
docker-compose up -d redis rabbitmq

# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON ..
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

## Development Workflow

### 1. Create a Feature Branch

```bash
# Update your main branch
git checkout main
git pull upstream main

# Create a feature branch
git checkout -b feature/your-feature-name
```

### 2. Make Your Changes

- Write clean, well-documented code
- Follow the coding standards (see below)
- Add tests for new functionality
- Update documentation as needed

### 3. Test Your Changes

```bash
# Run all tests
./run_tests.sh

# Run specific test suite
./build/config_test
./build/redis_cache_test
# etc.

# Check for memory leaks (optional)
valgrind --leak-check=full ./build/tts_playback_service
```

### 4. Commit Your Changes

```bash
# Stage your changes
git add .

# Commit with a descriptive message
git commit -m "feat: add new feature description"
```

**Commit Message Format**:
```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code formatting (no logic change)
- `refactor`: Code restructuring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Example**:
```
feat(cache): add TTL support for Redis cache

Implement time-to-live expiration for cached WAV files.
Configurable via CACHE_TTL environment variable.

Closes #123
```

### 5. Push and Create Pull Request

```bash
# Push to your fork
git push origin feature/your-feature-name

# Create a pull request on GitHub
```

## Coding Standards

### C++ Style Guide

#### Naming Conventions

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

#### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 100 characters maximum
- **Braces**: Same line for functions, new line for classes

```cpp
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

#### Best Practices

- Use RAII for resource management
- Prefer `std::unique_ptr` over raw pointers
- Use `const` where possible
- Pass large objects by const reference
- Use `std::move` for transfers
- Avoid `using namespace std`

### Documentation

- Add comments for complex logic
- Document public APIs with Doxygen-style comments
- Update README.md for user-facing changes
- Update technical docs in `docs/` for architectural changes

```cpp
/**
 * @brief Publishes a playback job to RabbitMQ queue
 * 
 * @param job The playback job containing text and WAV data
 * @param queue The queue name to publish to
 * @throws std::runtime_error if publish fails
 */
void publishJob(const PlaybackJob& job, const std::string& queue);
```

## Testing Requirements

### Test Coverage

All contributions must maintain 100% test coverage:

- Add unit tests for new functions
- Add integration tests for new features
- Update existing tests if behavior changes

### Writing Tests

```cpp
#include <gtest/gtest.h>
#include "your_class.h"

class YourClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
    
    void TearDown() override {
        // Cleanup code
    }
};

TEST_F(YourClassTest, DescriptiveTestName) {
    // Arrange
    YourClass obj;
    
    // Act
    auto result = obj.method();
    
    // Assert
    EXPECT_EQ(expected, result);
}
```

### Running Tests Locally

```bash
# All tests
./run_tests.sh

# Specific test with filter
./build/redis_cache_test --gtest_filter=*LRU*

# With verbose output
./build/config_test --gtest_print_time=1
```

## Pull Request Process

### Before Submitting

Ensure your PR meets these requirements:

- [ ] Code compiles without warnings
- [ ] All tests pass locally
- [ ] No memory leaks (run Valgrind)
- [ ] Code follows style guide
- [ ] Documentation is updated
- [ ] Commit messages are descriptive
- [ ] Branch is up to date with main

### PR Checklist

When creating a pull request:

1. **Title**: Clear, descriptive title
2. **Description**: Explain what and why
3. **Testing**: Describe how you tested
4. **Screenshots**: If UI changes (N/A for this project)
5. **Breaking Changes**: Note any breaking changes
6. **Related Issues**: Link related issues

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
How was this tested?

## Checklist
- [ ] Code compiles without warnings
- [ ] Tests pass locally
- [ ] Documentation updated
- [ ] No breaking changes (or documented)
```

### Review Process

1. **Automated Checks**: CI/CD pipeline runs automatically
2. **Code Review**: Maintainers review your code
3. **Feedback**: Address any requested changes
4. **Approval**: Once approved, maintainers will merge

## CI/CD Pipeline

### Automated Checks

Every pull request triggers:

1. **Build and Test** (~10 min)
   - Compiles the service
   - Runs 57 integration tests
   - Uploads test results

2. **Code Quality** (~30 sec)
   - Checks for trailing whitespace
   - Scans for TODO/FIXME
   - Verifies documentation

3. **Security Scan** (~2 min)
   - Trivy vulnerability scanning
   - Results in GitHub Security tab

4. **Docker Build** (~3 min)
   - Validates Docker image builds

### Pipeline Status

Check pipeline status:
- Go to **Actions** tab in GitHub
- View your PR's workflow run
- Check logs if any job fails

### Common CI Failures

#### Test Failures

**Cause**: Tests fail in CI but pass locally

**Solution**:
```bash
# Ensure services are running
docker-compose up -d redis rabbitmq

# Run tests exactly as CI does
cd build
ctest --output-on-failure --verbose
```

#### Build Failures

**Cause**: Missing dependencies or compilation errors

**Solution**:
```bash
# Clean build
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### Code Quality Failures

**Cause**: Trailing whitespace or missing docs

**Solution**:
```bash
# Remove trailing whitespace
find src include -name "*.cpp" -o -name "*.h" | xargs sed -i 's/[[:space:]]*$//'

# Verify documentation exists
ls docs/INDEX.md docs/api/API.md
```

## Development Tips

### Debugging

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with GDB
gdb ./tts_playback_service

# Check for memory leaks
valgrind --leak-check=full ./tts_playback_service
```

### Performance Profiling

```bash
# Build with profiling
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pg" ..
make

# Run and generate profile
./tts_playback_service
gprof tts_playback_service gmon.out > analysis.txt
```

### Docker Development

```bash
# Build Docker image
docker build -t tts-playback-service:dev .

# Run with docker-compose
docker-compose up --build

# View logs
docker-compose logs -f tts-playback
```

## Getting Help

- **Documentation**: Check [`docs/`](docs/) directory
- **Issues**: Search existing issues on GitHub
- **Discussions**: Use GitHub Discussions for questions
- **Contact**: Open an issue with `question` label

## License

By contributing, you agree that your contributions will be licensed under the AGPL-3.0 License.

---

**Thank you for contributing!** 🎉

Your contributions help make this project better for everyone.