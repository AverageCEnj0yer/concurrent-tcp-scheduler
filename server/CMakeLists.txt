cmake_minimum_required(VERSION 3.20)

project(concurrent_tcp_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(server_core STATIC
    src/concurrency/ThreadPool.cpp
    src/protocol/ProtocolJson.cpp
    src/server/NetUtils.cpp
    src/server/ClientSession.cpp
)

target_include_directories(server_core PUBLIC include)
target_link_libraries(server_core PUBLIC
    nlohmann_json::nlohmann_json
    Threads::Threads
)
target_compile_options(server_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
)
