# Unit tests for TTS Playback Service

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${PULSE_INCLUDE_DIRS})
include_directories(${EVENT_INCLUDE_DIRS})

# Test utilities and mocks
add_library(test_mocks STATIC
    mocks/mock_redis.cpp
    mocks/mock_rabbitmq.cpp
    mocks/mock_pulseaudio.cpp
)

target_link_libraries(test_mocks
    GTest::gtest
    GTest::gmock
)

# Config tests
add_executable(config_test
    test_config.cpp
)

target_link_libraries(config_test
    GTest::gtest_main
    GTest::gmock
)

# Redis cache tests
add_executable(redis_cache_test
    test_redis_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/redis_cache.cpp
)

target_link_libraries(redis_cache_test
    GTest::gtest_main
    GTest::gmock
    hiredis
    redis++
    Threads::Threads
)

# RabbitMQ client tests
add_executable(rabbitmq_client_test
    test_rabbitmq_client.cpp
    ${CMAKE_SOURCE_DIR}/src/rabbitmq_client.cpp
)

target_link_libraries(rabbitmq_client_test
    GTest::gtest_main
    GTest::gmock
    amqpcpp
    ${EVENT_LIBRARIES}
    Threads::Threads
)

# Audio player tests
add_executable(audio_player_test
    test_audio_player.cpp
    ${CMAKE_SOURCE_DIR}/src/audio_player.cpp
)

target_link_libraries(audio_player_test
    GTest::gtest_main
    GTest::gmock
    ${PULSE_LIBRARIES}
)

# API server tests
add_executable(api_server_test
    test_api_server.cpp
    ${CMAKE_SOURCE_DIR}/src/api_server.cpp
)

target_link_libraries(api_server_test
    GTest::gtest_main
    GTest::gmock
    Threads::Threads
)

# Integration tests
add_executable(integration_test
    test_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/redis_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/rabbitmq_client.cpp
    ${CMAKE_SOURCE_DIR}/src/audio_player.cpp
    ${CMAKE_SOURCE_DIR}/src/api_server.cpp
)

target_link_libraries(integration_test
    GTest::gtest_main
    GTest::gmock
    Threads::Threads
    ${PULSE_LIBRARIES}
    ${EVENT_LIBRARIES}
    amqpcpp
    hiredis
    redis++
)

target_compile_definitions(integration_test PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

# Register tests with CTest
gtest_discover_tests(config_test)
gtest_discover_tests(redis_cache_test)
gtest_discover_tests(rabbitmq_client_test)
gtest_discover_tests(audio_player_test)
gtest_discover_tests(api_server_test)
gtest_discover_tests(integration_test)

# All tests target
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS 
        config_test
        redis_cache_test
        rabbitmq_client_test
        audio_player_test
        api_server_test
        integration_test
)
