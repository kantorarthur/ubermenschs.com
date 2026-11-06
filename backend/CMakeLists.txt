cmake_minimum_required(VERSION 3.21)

project(ubermenschs_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Also build the tests (needs GoogleTest from vcpkg)" OFF)

# Libraries come from vcpkg (see vcpkg.json).
find_package(Crow CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
# OpenSSL (Web Push cryptography) and libcurl (sending to the push services) come from the system:
# apt install libssl-dev libcurl4-openssl-dev (the Dockerfile does it).
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# vcpkg builds redis-plus-plus as a static library on Linux and a shared one on Windows.
if(TARGET redis++::redis++_static)
    set(REDIS_PLUS_PLUS redis++::redis++_static)
else()
    set(REDIS_PLUS_PLUS redis++::redis++)
endif()

if(MSVC)
    set(WARNINGS /W4 /permissive-)
else()
    set(WARNINGS -Wall -Wextra -Wpedantic)
endif()

# Everything except main(), so the server and the tests share the same code.
add_library(core STATIC
    src/auth_routes.cpp
    src/config.cpp
    src/crypto.cpp
    src/datetime.cpp
    src/event_routes.cpp
    src/event_store.cpp
    src/http.cpp
    src/preset_routes.cpp
    src/preset_store.cpp
    src/push_routes.cpp
    src/push_sender.cpp
    src/push_store.cpp
    src/rate_limiter.cpp
    src/redis_client.cpp
    src/reminder_routes.cpp
    src/reminder_scheduler.cpp
    src/reminder_store.cpp
    src/text.cpp
    src/user_store.cpp
    src/webpush_crypto.cpp
)
target_include_directories(core PUBLIC src)
target_link_libraries(core PUBLIC Crow::Crow ${REDIS_PLUS_PLUS} unofficial-sodium::sodium
                                  OpenSSL::Crypto CURL::libcurl)
target_compile_options(core PRIVATE ${WARNINGS})

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE core)
target_compile_options(server PRIVATE ${WARNINGS})

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)

    add_executable(tests
        tests/crypto_test.cpp
        tests/datetime_test.cpp
        tests/event_store_test.cpp
        tests/http_test.cpp
        tests/preset_store_test.cpp
        tests/push_store_test.cpp
        tests/rate_limiter_test.cpp
        tests/reminder_scheduler_test.cpp
        tests/reminder_store_test.cpp
        tests/session_store_test.cpp
        tests/text_test.cpp
        tests/user_store_test.cpp
        tests/webpush_crypto_test.cpp
    )
    target_link_libraries(tests PRIVATE core GTest::gtest GTest::gtest_main)
    target_compile_options(tests PRIVATE ${WARNINGS})

    add_test(NAME tests COMMAND tests)
endif()
