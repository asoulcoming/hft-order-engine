cmake_minimum_required(VERSION 3.20)
project(hft-order-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GoogleTest via FetchContent（断网模式：已下载过就不联网检查）
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# ── Engine library ──────────────────────────────────────────
add_library(hft_engine STATIC
    src/book/order_book.cpp
    src/io/parser.cpp
    src/io/formatter.cpp
)
target_include_directories(hft_engine PUBLIC include)
target_compile_options(hft_engine PRIVATE -Wall -Wextra -Wpedantic)

# ── Main executable ─────────────────────────────────────────
add_executable(hft_order_engine src/main.cpp)
target_link_libraries(hft_order_engine PRIVATE hft_engine)

# ── Tests ───────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
