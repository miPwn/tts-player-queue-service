cmake_minimum_required(VERSION 3.20)
project(tts_playback_service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PULSE REQUIRED libpulse libpulse-simple)
pkg_check_modules(EVENT REQUIRED libevent)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${PULSE_INCLUDE_DIRS})
include_directories(${EVENT_INCLUDE_DIRS})

add_executable(tts_playback_service
    src/main.cpp
    src/redis_cache.cpp
    src/rabbitmq_client.cpp
    src/audio_player.cpp
    src/api_server.cpp
)

target_link_libraries(tts_playback_service
    Threads::Threads
    ${PULSE_LIBRARIES}
    ${EVENT_LIBRARIES}
    amqpcpp
    hiredis
    redis++
)

target_compile_definitions(tts_playback_service PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

install(TARGETS tts_playback_service
    RUNTIME DESTINATION bin
)

option(BUILD_TESTING "Build unit tests" ON)

if(BUILD_TESTING)
    enable_testing()
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    include(GoogleTest)
    
    add_subdirectory(tests)
    
    message(STATUS "Unit tests enabled")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
