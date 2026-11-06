cmake_minimum_required(VERSION 3.21)

project(ubermenschs_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Libraries come from vcpkg (see vcpkg.json).
find_package(Crow CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)

add_executable(server
    src/main.cpp
    src/config.cpp
    src/redis_client.cpp
)

# vcpkg builds redis-plus-plus as a static library on Linux and a shared one on Windows.
if(TARGET redis++::redis++_static)
    set(REDIS_PLUS_PLUS redis++::redis++_static)
else()
    set(REDIS_PLUS_PLUS redis++::redis++)
endif()

target_link_libraries(server PRIVATE Crow::Crow ${REDIS_PLUS_PLUS})

if(MSVC)
    target_compile_options(server PRIVATE /W4 /permissive-)
else()
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
endif()
