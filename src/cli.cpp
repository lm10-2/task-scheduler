#include "cli.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace cli::color;

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
static std::string repeat(const std::string& s, int n) {
    std::string r;
    for (int i = 0; i < n; ++i) r += s;
    return r;
}

static void hline(int w = 60) {
    std::cout << DIM << repeat("─", w) << RESET << '\n';
}

static void section(const std::string& title) {
    std::cout << '\n' << BOLD << CYAN << "  ┌─ " << title << " " << RESET
              << DIM << repeat("─", std::max(0, 52 - (int)title.size()))
              << RESET << '\n';
}

// ─────────────────────────────────────────────
//  Banner
// ─────────────────────────────────────────────
void cli::print_banner() {
    std::cout << BOLD << CYAN
<< R"(
  ╔══════════════════════════════════════════════════════╗
  ║        DAG Task Scheduler  — v1.0                   ║
  ║   Kahn's Algorithm · Cycle Detection · O(V+E)       ║
  ╚══════════════════════════════════════════════════════╝
)" << RESET;
}

// ─────────────────────────────────────────────
//  Main result printer
// ─────────────────────────────────────────────
void cli::print_result(const ScheduleResult& r,
                       const TaskScheduler&  sched,
                       bool verbose)
{
    if (!r.success) {
        // ── Cycle report ──────────────────────────
        std::cout << '\n' << BOLD << RED
                  << "  ✖  CYCLE DETECTED — scheduling aborted\n" << RESET;
        hline();
        std::cout << RED << "  Circular dependency path:\n" << RESET;
        for (size_t i = 0; i < r.cycle.size(); ++i) {
            if (i) std::cout << RED << "  →  " << RESET;
            std::cout << YELLOW << r.cycle[i] << RESET;
        }
        std::cout << RED << "  →  (back to start)\n" << RESET;
        hline();
        return;
    }

    // ── Summary bar ───────────────────────────────
    std::cout << '\n' << BOLD << GREEN
              << "  ✔  Schedule computed successfully\n" << RESET;
    hline(60);
    std::cout << "  Tasks : " << BOLD << sched.task_count() << RESET
              << "    Edges : " << BOLD << sched.edge_count() << RESET
              << "    Waves : " << BOLD << r.parallel_waves.size() << RESET
              << "    Bottlenecks : " << BOLD << r.bottlenecks.size() << RESET
              << '\n';
    hline(60);

    // ── Flat execution order ──────────────────────
    if (verbose) {
        section("Execution Order  (flat topological)");
        std::cout << "  ";
        for (size_t i = 0; i < r.execution_order.size(); ++i) {
            if (i) std::cout << DIM << " → " << RESET;
            std::cout << WHITE << r.execution_order[i] << RESET;
        }
        std::cout << '\n';
    }

    // ── Parallel waves ────────────────────────────
    section("Parallel Execution Waves");
    for (size_t wi = 0; wi < r.parallel_waves.size(); ++wi) {
        const auto& wave = r.parallel_waves[wi];
        bool is_bottleneck = (wave.size() == 1);

        std::cout << "  Wave " << BOLD << std::setw(2) << (wi + 1) << RESET
                  << "  ";

        if (is_bottleneck)
            std::cout << YELLOW << "[BOTTLENECK] " << RESET;
        else
            std::cout << GREEN  << "[parallel×"
                      << wave.size() << "]   " << RESET;

        for (size_t ti = 0; ti < wave.size(); ++ti) {
            if (ti) std::cout << DIM << "  ‖  " << RESET;
            std::cout << WHITE << wave[ti] << RESET;
        }
        std::cout << '\n';
    }

    // ── Bottlenecks list ──────────────────────────
    if (!r.bottlenecks.empty()) {
        section("Bottleneck Stages");
        for (auto& id : r.bottlenecks)
            std::cout << "  " << YELLOW << "⚠  " << RESET << id << '\n';
    }

    // ── Critical path ─────────────────────────────
    section("Critical Path  (longest dependency chain)");
    std::cout << "  Duration : " << BOLD << MAGENTA
              << r.critical_path_duration << " ms" << RESET << '\n';
    std::cout << "  Path     : ";
    for (size_t i = 0; i < r.critical_path.size(); ++i) {
        if (i) std::cout << MAGENTA << " ──▶ " << RESET;
        std::cout << BOLD << r.critical_path[i] << RESET;
    }
    std::cout << '\n';

    // ── Per-task dependency detail ────────────────
    if (verbose) {
        section("Task Detail");
        std::cout << "  " << std::left
                  << std::setw(14) << "ID"
                  << std::setw(22) << "Name"
                  << std::setw(10) << "Duration"
                  << std::setw(10) << "Priority"
                  << "Depends on\n";
        std::cout << "  " << repeat("─", 72) << '\n';
        for (auto& id : r.execution_order) {
            auto deps = sched.dependencies_of(id);
            std::cout << "  "
                      << std::setw(14) << id
                      << std::setw(22) << id   // name == id in CLI demo
                      << std::setw(10) << (std::to_string(0) + " ms")
                      << std::setw(10) << "—"
                      << (deps.empty() ? "(none)" : "");
            for (size_t i = 0; i < deps.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << deps[i];
            }
            std::cout << '\n';
        }
    }

    hline(60);
}

// ─────────────────────────────────────────────
//  Graph visualiser (ASCII adjacency list)
// ─────────────────────────────────────────────
void cli::print_graph(const TaskScheduler& sched) {
    section("Adjacency List");
    // We'll iterate via execution order for a tidy display
    auto r = sched.schedule();
    if (!r.success) { std::cout << RED << "  (cycle present)\n" << RESET; return; }
    for (auto& id : r.execution_order) {
        auto deps = sched.dependents_of(id);
        std::cout << "  " << BOLD << std::setw(14) << id << RESET
                  << DIM << " ──▶  " << RESET;
        if (deps.empty()) std::cout << DIM << "(terminal)" << RESET;
        for (size_t i = 0; i < deps.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << CYAN << deps[i] << RESET;
        }
        std::cout << '\n';
    }
}

// ─────────────────────────────────────────────
//  Help text
// ─────────────────────────────────────────────
void cli::print_help() {
    std::cout << BOLD << "\n  Commands\n" << RESET;
    hline(50);
    auto row = [](const std::string& cmd, const std::string& desc) {
        std::cout << "  " << CYAN << std::left << std::setw(28) << cmd
                  << RESET << desc << '\n';
    };
    row("add <id> [name] [dur] [pri]", "Add a task");
    row("dep <from> <to>",             "Add dependency edge");
    row("remove <id>",                 "Remove a task");
    row("schedule",                    "Run Kahn's algorithm");
    row("graph",                       "Show adjacency list");
    row("demo basic",                  "Load a basic demo");
    row("demo complex",                "Load a complex demo");
    row("demo cycle",                  "Load a cycle demo");
    row("demo build",                  "Load a build-system demo");
    row("save <file>",                 "Save graph to file");
    row("load <file>",                 "Load graph from file");
    row("clear",                       "Clear all tasks");
    row("help",                        "Show this help");
    row("exit / quit",                 "Quit");
    hline(50);
}
