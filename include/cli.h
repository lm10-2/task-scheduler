#pragma once
#include "scheduler.h"
#include <string>

// ─────────────────────────────────────────────
//  All terminal-output helpers live here so
//  scheduler.h stays dependency-free.
// ─────────────────────────────────────────────
namespace cli {

// ANSI colour helpers (auto-disabled on Windows without VT mode)
namespace color {
    constexpr const char* RESET  = "\033[0m";
    constexpr const char* BOLD   = "\033[1m";
    constexpr const char* RED    = "\033[31m";
    constexpr const char* GREEN  = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* CYAN   = "\033[36m";
    constexpr const char* MAGENTA= "\033[35m";
    constexpr const char* WHITE  = "\033[97m";
    constexpr const char* DIM    = "\033[2m";
}

void print_banner();
void print_result(const ScheduleResult& r,
                  const TaskScheduler&  sched,
                  bool verbose = true);
void print_help();
void print_graph(const TaskScheduler& sched);

} // namespace cli
