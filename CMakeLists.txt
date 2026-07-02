cmake_minimum_required(VERSION 3.16)
project(TaskScheduler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ─────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# ── Include dirs ───────────────────────────────────────────
include_directories(include)

# ── Executable ─────────────────────────────────────────────
add_executable(scheduler
    src/main.cpp
    src/scheduler.cpp
    src/cli.cpp
)

# ── Optional: test binary ──────────────────────────────────
add_executable(scheduler_tests
    tests/test_main.cpp
    src/scheduler.cpp
    src/cli.cpp
)
target_include_directories(scheduler_tests PRIVATE include)

# ── Install target ─────────────────────────────────────────
install(TARGETS scheduler DESTINATION bin)
