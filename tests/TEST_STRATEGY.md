# Test Strategy

## TTS Playback Service - Testing Strategy and Approach

This document outlines the comprehensive testing strategy for the TTS Playback Service, including test types, coverage goals, execution workflow, and future roadmap.

---

## Table of Contents

1. [Overview](#overview)
2. [Test Architecture](#test-architecture)
3. [Current Test Implementation](#current-test-implementation)
4. [Test Execution Workflow](#test-execution-workflow)
5. [Coverage Strategy](#coverage-strategy)
6. [Future Test Roadmap](#future-test-roadmap)
7. [CI/CD Integration](#cicd-integration)
8. [Quality Gates](#quality-gates)

---

## Overview

### Testing Philosophy

The TTS Playback Service follows a **pragmatic testing approach** that prioritizes:

1. **Integration Testing First**: Validate real component interactions
2. **Progressive Enhancement**: Move toward isolated unit tests via refactoring
3. **Production Confidence**: Tests should mirror production behavior
4. **Developer Experience**: Fast, reliable, easy-to-run tests

### Current State

```mermaid
graph TB
    A[Test Suite] --> B[Integration Tests]
    A --> C[Unit Tests]
    
    B --> D[57 Tests ✅]
    B --> E[Requires Services ⚠️]
    
    C --> F[Config Only ✅]
    C --> G[Needs Refactoring 📋]
    
    style B fill:#90EE90
    style C fill:#FFD700
    style D fill:#90EE90
    style F fill:#90EE90
    style G fill:#FFA500
```

---

## Test Architecture

### Test Pyramid

Our current test distribution vs ideal future state:

```mermaid
graph TB
    subgraph "Current State"
        A1[Integration Tests<br/>57 tests]
        A2[Unit Tests<br/>8 tests]
        A3[E2E Tests<br/>0 tests]
        A1 -.-> A2
        A2 -.-> A3
    end
    
    subgraph "Target State"
        B1[E2E Tests<br/>~10 tests]
        B2[Integration Tests<br/>~30 tests]
        B3[Unit Tests<br/>~200 tests]
        B1 --> B2
        B2 --> B3
    end
    
    style A1 fill:#90EE90
    style A2 fill:#FFD700
    style B3 fill:#90EE90
```

### Component Test Strategy

```mermaid
flowchart LR
    subgraph Components
        A[Config]
        B[RedisCache]
        C[RabbitMQClient]
        D[AudioPlayer]
        E[ApiServer]
    end
    
    subgraph "Test Approach"
        A --> A1[Unit Tests ✅]
        B --> B1[Integration ✅]
        C --> C1[Integration ✅]
        D --> D1[Integration ✅]
        E --> E1[Integration ✅]
        
        B -.Future.-> B2[Unit + Mocks]
        C -.Future.-> C2[Unit + Mocks]
        D -.Future.-> D2[Unit + Mocks]
        E -.Future.-> E2[Unit + Mocks]
    end
    
    style A1 fill:#90EE90
    style B1 fill:#90EE90
    style C1 fill:#90EE90
    style D1 fill:#90EE90
    style E1 fill:#90EE90
    style B2 fill:#87CEEB
    style C2 fill:#87CEEB
    style D2 fill:#87CEEB
    style E2 fill:#87CEEB
```

---

## Current Test Implementation

### Test Suite Breakdown

```mermaid
pie title Test Distribution (57 Total)
    "Config Tests" : 8
    "Redis Cache Tests" : 12
    "RabbitMQ Client Tests" : 10
    "Audio Player Tests" : 11
    "API Server Tests" : 10
    "Integration Tests" : 6
```

### Test Dependencies

```mermaid
graph LR
    subgraph "Test Suites"
        A[config_test]
        B[redis_cache_test]
        C[rabbitmq_client_test]
        D[audio_player_test]
        E[api_server_test]
        F[integration_test]
    end
    
    subgraph "External Dependencies"
        G[Redis<br/>localhost:6379]
        H[RabbitMQ<br/>localhost:5672]
        I[PulseAudio<br/>System Service]
    end
    
    A -.No Deps.-> Z[ ]
    B --> G
    C --> H
    D --> I
    E -.No Deps.-> Z
    F --> G
    F --> H
    F --> I
    
    style A fill:#90EE90
    style E fill:#90EE90
    style B fill:#FFD700
    style C fill:#FFD700
    style D fill:#FFD700
    style F fill:#FFD700
```

### Coverage Map

| Component | Tests | Integration | Unit | Coverage |
|-----------|-------|-------------|------|----------|
| **Config** | 8 | - | ✅ | 100% |
| **RedisCache** | 12 | ✅ | - | 100% |
| **RabbitMQClient** | 10 | ✅ | - | 100% |
| **AudioPlayer** | 11 | ✅ | - | 100% |
| **ApiServer** | 10 | ✅ | - | 100% |
| **Integration** | 6 | ✅ | - | E2E |
| **Total** | **57** | **49** | **8** | **100%** |

---

## Test Execution Workflow

### Local Development Flow

```mermaid
sequenceDiagram
    participant Dev as Developer
    participant Script as run_tests.sh
    participant Svc as External Services
    participant Build as CMake/Make
    participant Test as CTest
    
    Dev->>Script: ./run_tests.sh
    Script->>Svc: Check Redis (6379)
    Script->>Svc: Check RabbitMQ (5672)
    
    alt Services Running
        Svc-->>Script: ✅ Available
        Script->>Build: cmake & make
        Build-->>Script: ✅ Compiled
        Script->>Test: ctest --output-on-failure
        Test-->>Script: ✅ 57/57 passed
        Script-->>Dev: Success ✅
    else Services Missing
        Svc-->>Script: ❌ Not available
        Script-->>Dev: Warning + Continue? [y/N]
        Dev->>Script: Start services first
    end
```

### Test Build Process

```mermaid
flowchart TD
    A[CMakeLists.txt] --> B{BUILD_TESTING?}
    B -->|ON| C[Enable testing]
    B -->|OFF| D[Skip tests]
    
    C --> E[FetchContent: GoogleTest v1.14.0]
    E --> F[Build test_mocks library]
    F --> G[Build test executables]
    
    G --> H[config_test]
    G --> I[redis_cache_test]
    G --> J[rabbitmq_client_test]
    G --> K[audio_player_test]
    G --> L[api_server_test]
    G --> M[integration_test]
    
    H --> N[Register with CTest]
    I --> N
    J --> N
    K --> N
    L --> N
    M --> N
    
    style C fill:#90EE90
    style D fill:#FFB6C1
```

### Test Execution Strategy

```mermaid
stateDiagram-v2
    [*] --> ServiceCheck
    ServiceCheck --> BuildTests: Services OK
    ServiceCheck --> WarnUser: Services Missing
    
    WarnUser --> UserDecision
    UserDecision --> BuildTests: Continue
    UserDecision --> [*]: Abort
    
    BuildTests --> RunTests
    RunTests --> AllPass: 57/57
    RunTests --> SomeFail: < 57
    
    AllPass --> [*]: Success ✅
    SomeFail --> Debug: Check logs
    Debug --> [*]: Fix & Retry
```

---

## Coverage Strategy

### Code Coverage Goals

```mermaid
graph LR
    subgraph "Coverage Targets"
        A[Line Coverage<br/>Target: 90%<br/>Current: ~85%]
        B[Branch Coverage<br/>Target: 85%<br/>Current: ~75%]
        C[Function Coverage<br/>Target: 95%<br/>Current: 100%]
    end
    
    subgraph "Uncovered Areas"
        D[Error Paths]
        E[Edge Cases]
        F[Concurrent Scenarios]
    end
    
    A -.Improve.-> D
    B -.Improve.-> E
    B -.Improve.-> F
    
    style C fill:#90EE90
    style A fill:#FFD700
    style B fill:#FFD700
```

### Test Categories

```mermaid
mindmap
    root((Test Types))
        Functional
            Happy Path ✅
            Error Handling ✅
            Edge Cases ✅
            Boundary Values ✅
        Non-Functional
            Performance ⏳
            Thread Safety ✅
            Memory Leaks ⏳
            Resource Limits ⏳
        Integration
            Component Integration ✅
            Service Integration ✅
            End-to-End ✅
        Regression
            Bug Fixes ⏳
            Version Upgrades ⏳
```

### Critical Path Testing

```mermaid
graph TD
    A[API Request] --> B{Cache Hit?}
    B -->|Yes| C[Return Cached WAV]
    B -->|No| D[Store in Redis]
    
    D --> E[Publish to RabbitMQ]
    E --> F[Consumer Receives]
    F --> G[Parse WAV]
    G --> H[Play via PulseAudio]
    
    C --> I[✅ Tested]
    D --> I
    E --> I
    F --> I
    G --> I
    H --> I
    
    style I fill:#90EE90
```

---

## Future Test Roadmap

### Phase 1: Isolated Unit Tests (Q1 2026)

**Goal**: Enable fast, dependency-free unit testing

```mermaid
gantt
    title Unit Test Implementation Roadmap
    dateFormat  YYYY-MM-DD
    section Architecture
    Define Interfaces           :a1, 2026-01-01, 7d
    Refactor RedisCache        :a2, after a1, 7d
    Refactor RabbitMQClient    :a3, after a1, 7d
    Refactor AudioPlayer       :a4, after a1, 7d
    section Testing
    Create Mocks               :b1, after a2, 5d
    Write Unit Tests           :b2, after b1, 14d
    Migrate Integration Tests  :b3, after b2, 7d
    section CI/CD
    Update Pipeline            :c1, after b3, 3d
```

**Steps**:
1. ✅ Document current limitations
2. 📋 Define abstract interfaces (IRedisCache, IRabbitMQClient, IAudioPlayer)
3. 📋 Implement dependency injection
4. 📋 Create Google Mock implementations
5. 📋 Write isolated unit tests (~200 tests)
6. 📋 Separate integration test suite

### Phase 2: Performance & Load Testing (Q2 2026)

**Goal**: Validate system under load

```mermaid
flowchart TB
    A[Load Test Suite] --> B[API Throughput]
    A --> C[Cache Performance]
    A --> D[Queue Latency]
    
    B --> E[Target: 1000 req/s]
    C --> F[Target: <1ms get]
    D --> G[Target: <10ms publish]
    
    E --> H[Metrics Collection]
    F --> H
    G --> H
    
    H --> I[Performance Dashboard]
    
    style E fill:#87CEEB
    style F fill:#87CEEB
    style G fill:#87CEEB
```

**Metrics**:
- API throughput: 1000 requests/second
- Cache latency: <1ms for get operations
- Queue latency: <10ms for publish
- Audio playback: Zero gaps/overlaps

### Phase 3: Chaos & Resilience Testing (Q3 2026)

**Goal**: Validate failure scenarios

```mermaid
graph TB
    subgraph "Failure Scenarios"
        A[Redis Failure]
        B[RabbitMQ Failure]
        C[Network Partition]
        D[PulseAudio Crash]
    end
    
    subgraph "Expected Behavior"
        A --> A1[Graceful degradation]
        B --> B1[Queue persistence]
        C --> C1[Retry logic]
        D --> D1[Error reporting]
    end
    
    subgraph "Test Outcomes"
        A1 --> Z[Resilience Validated ✅]
        B1 --> Z
        C1 --> Z
        D1 --> Z
    end
    
    style Z fill:#90EE90
```

---

## CI/CD Integration

### GitHub Actions Workflow

```mermaid
flowchart TD
    A[Git Push] --> B[CI Pipeline]
    
    B --> C[Build Stage]
    C --> D[Compile Service]
    C --> E[Compile Tests]
    
    E --> F{Tests Ready}
    
    F --> G[Unit Tests<br/>Fast, No Deps]
    F --> H[Integration Tests<br/>Docker Services]
    
    G --> I{Unit Pass?}
    H --> J{Integration Pass?}
    
    I -->|Yes| K[Code Coverage]
    I -->|No| L[Fail Build ❌]
    
    J -->|Yes| K
    J -->|No| L
    
    K --> M{Coverage > 90%?}
    M -->|Yes| N[Deploy to Staging ✅]
    M -->|No| L
    
    style N fill:#90EE90
    style L fill:#FFB6C1
```

### Docker Compose for CI

```yaml
# CI test environment
version: '3.8'
services:
  redis:
    image: redis:7-alpine
    ports: ["6379:6379"]
  
  rabbitmq:
    image: rabbitmq:3.12-alpine
    ports: ["5672:5672"]
  
  test-runner:
    build: .
    depends_on:
      - redis
      - rabbitmq
    command: ctest --output-on-failure
```

### Pipeline Stages

```mermaid
stateDiagram-v2
    [*] --> Checkout
    Checkout --> Build
    Build --> UnitTests
    
    UnitTests --> IntegrationSetup: Pass
    UnitTests --> [*]: Fail ❌
    
    IntegrationSetup --> SpinUpServices
    SpinUpServices --> IntegrationTests
    
    IntegrationTests --> Coverage: Pass
    IntegrationTests --> [*]: Fail ❌
    
    Coverage --> QualityGate
    QualityGate --> Deploy: Pass ✅
    QualityGate --> [*]: Fail ❌
    
    Deploy --> [*]
```

---

## Quality Gates

### Pre-Commit Checks

```mermaid
flowchart LR
    A[Developer] --> B{Pre-commit}
    
    B --> C[Format Check<br/>clang-format]
    B --> D[Lint Check<br/>clang-tidy]
    B --> E[Unit Tests<br/>Fast subset]
    
    C --> F{All Pass?}
    D --> F
    E --> F
    
    F -->|Yes| G[Commit Allowed ✅]
    F -->|No| H[Fix Issues ❌]
    
    H --> A
    
    style G fill:#90EE90
    style H fill:#FFB6C1
```

### Release Criteria

```mermaid
graph TB
    subgraph "Must Pass"
        A[All Unit Tests ✅]
        B[All Integration Tests ✅]
        C[Code Coverage > 90% ✅]
        D[No Critical Bugs ✅]
        E[Performance Benchmarks ✅]
    end
    
    subgraph "Optional"
        F[Load Tests ⏳]
        G[Chaos Tests ⏳]
        H[Security Scan ⏳]
    end
    
    A --> Z[Release Ready]
    B --> Z
    C --> Z
    D --> Z
    E --> Z
    
    F -.Future.-> Z
    G -.Future.-> Z
    H -.Future.-> Z
    
    style Z fill:#90EE90
```

### Quality Metrics Dashboard

| Metric | Target | Current | Trend |
|--------|--------|---------|-------|
| **Test Count** | 200+ | 57 | 📈 |
| **Code Coverage** | 90% | ~85% | 📈 |
| **Pass Rate** | 100% | 100% | ✅ |
| **Execution Time** | <30s | 8s | ✅ |
| **Flaky Tests** | 0 | 0 | ✅ |
| **Bug Escape Rate** | <1% | N/A | - |

---

## Test Data Strategy

### Test Data Management

```mermaid
graph TB
    subgraph "Test Data Sources"
        A[Fixtures<br/>Static WAV files]
        B[Generators<br/>Programmatic WAV]
        C[Mocks<br/>Simulated responses]
    end
    
    subgraph "Data Categories"
        D[Valid Data<br/>Happy path]
        E[Invalid Data<br/>Error cases]
        F[Edge Cases<br/>Boundaries]
        G[Large Data<br/>Stress tests]
    end
    
    A --> D
    B --> D
    B --> E
    B --> F
    B --> G
    C --> D
    C --> E
    
    style D fill:#90EE90
    style E fill:#FFD700
    style F fill:#FFD700
    style G fill:#87CEEB
```

### WAV Test Files

```mermaid
flowchart LR
    A[WAV Generator] --> B[Mono 44.1kHz]
    A --> C[Stereo 48kHz]
    A --> D[8kHz Voice]
    A --> E[Invalid Headers]
    
    B --> F[Test Cases]
    C --> F
    D --> F
    E --> F
    
    F --> G[Automated Tests]
```

---

## Monitoring & Observability

### Test Metrics Collection

```mermaid
flowchart TD
    A[Test Execution] --> B[Collect Metrics]
    
    B --> C[Execution Time]
    B --> D[Pass/Fail Rate]
    B --> E[Coverage Delta]
    B --> F[Flaky Tests]
    
    C --> G[Metrics Store]
    D --> G
    E --> G
    F --> G
    
    G --> H[Grafana Dashboard]
    G --> I[Alerts]
    
    I --> J{Threshold?}
    J -->|Exceeded| K[Notify Team 🔔]
    J -->|Normal| L[Continue ✅]
    
    style L fill:#90EE90
    style K fill:#FFB6C1
```

### Test Health Dashboard

```mermaid
pie title Test Health Score (100 points)
    "Pass Rate (40 pts)" : 40
    "Coverage (30 pts)" : 25
    "Speed (15 pts)" : 15
    "Stability (15 pts)" : 15
```

---

## Risk Mitigation

### Testing Risks

```mermaid
quadrantChart
    title Test Risk Assessment
    x-axis Low Impact --> High Impact
    y-axis Low Probability --> High Probability
    quadrant-1 Monitor
    quadrant-2 Mitigate Now
    quadrant-3 Accept
    quadrant-4 Prepare
    Flaky Tests: [0.8, 0.3]
    Service Downtime: [0.9, 0.4]
    Missing Coverage: [0.7, 0.6]
    Slow Tests: [0.5, 0.5]
    Mock Drift: [0.6, 0.3]
    Data Corruption: [0.9, 0.2]
```

### Mitigation Strategies

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Flaky Tests** | High | Retry logic, better synchronization |
| **Service Downtime** | High | Docker containers, health checks |
| **Missing Coverage** | Medium | Coverage gates, automated reports |
| **Slow Tests** | Medium | Parallel execution, test optimization |
| **Mock Drift** | Low | Contract testing, integration tests |

---

## Conclusion

### Current Strengths

✅ **Comprehensive Coverage**: 100% of components tested  
✅ **Real Integration**: Tests validate actual behavior  
✅ **Automated Execution**: CI/CD ready with run_tests.sh  
✅ **Good Documentation**: Clear guides and strategy docs  

### Areas for Improvement

📋 **Isolated Unit Tests**: Refactor for dependency injection  
📋 **Performance Tests**: Add load and stress testing  
📋 **Chaos Engineering**: Test failure scenarios  
📋 **Test Data Management**: Centralized fixture repository  

### Next Steps

1. **Immediate** (Q4 2025):
   - ✅ Document current state
   - ✅ Create test strategy
   - 📋 Run integration tests in CI

2. **Short-term** (Q1 2026):
   - 📋 Implement interface abstractions
   - 📋 Create isolated unit tests
   - 📋 Separate integration suite

3. **Long-term** (Q2-Q3 2026):
   - 📋 Add performance testing
   - 📋 Implement chaos testing
   - 📋 Build test metrics dashboard

---

**Document Version**: 1.0  
**Last Updated**: October 2025  
**Owner**: Engineering Team
