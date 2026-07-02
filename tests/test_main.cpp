// ─────────────────────────────────────────────────────────
//  Minimal self-contained test runner (no external libs)
// ─────────────────────────────────────────────────────────
#include "scheduler.h"
#include "demos.h"
#include <iostream>
#include <cassert>
#include <string>

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) void test_##name(); \
    struct _Reg_##name { _Reg_##name() { \
        ++tests_run; \
        try { test_##name(); ++tests_passed; \
              std::cout << "  \033[32m✔\033[0m  " #name "\n"; } \
        catch (const std::exception& e) { \
              std::cout << "  \033[31m✖\033[0m  " #name " — " << e.what() << "\n"; } \
        catch (...) { \
              std::cout << "  \033[31m✖\033[0m  " #name " — unknown error\n"; } \
    } } _reg_##name; \
    void test_##name()

#define ASSERT(cond) do { if (!(cond)) throw std::runtime_error("ASSERT failed: " #cond); } while(0)
#define ASSERT_EQ(a,b) do { if ((a)!=(b)) throw std::runtime_error("ASSERT_EQ failed: " #a " != " #b); } while(0)

// ─────────────────────────────────────────────
//  Tests
// ─────────────────────────────────────────────

TEST(add_task) {
    TaskScheduler s;
    ASSERT(s.add_task({"A", "Task A", 100, 0}));
    ASSERT(!s.add_task({"A", "Dup",   100, 0}));   // duplicate
    ASSERT_EQ(s.task_count(), 1u);
}

TEST(add_dependency) {
    TaskScheduler s;
    s.add_task({"A", "A", 100, 0});
    s.add_task({"B", "B", 100, 0});
    ASSERT(s.add_dependency("A", "B"));
    ASSERT(!s.add_dependency("A", "B"));            // duplicate edge
    ASSERT(!s.add_dependency("X", "B"));            // unknown task
    ASSERT_EQ(s.edge_count(), 1u);
}

TEST(linear_schedule) {
    TaskScheduler s;
    s.add_task({"A", "A", 10, 0});
    s.add_task({"B", "B", 10, 0});
    s.add_task({"C", "C", 10, 0});
    s.add_dependency("A", "B");
    s.add_dependency("B", "C");
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT_EQ(r.execution_order.size(), 3u);
    ASSERT_EQ(r.execution_order[0], "A");
    ASSERT_EQ(r.execution_order[2], "C");
    ASSERT_EQ(r.parallel_waves.size(), 3u);          // no parallelism
}

TEST(parallel_tasks) {
    TaskScheduler s;
    s.add_task({"root", "root", 10, 0});
    s.add_task({"A",    "A",    10, 0});
    s.add_task({"B",    "B",    10, 0});
    s.add_task({"end",  "end",  10, 0});
    s.add_dependency("root", "A");
    s.add_dependency("root", "B");
    s.add_dependency("A",    "end");
    s.add_dependency("B",    "end");
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT_EQ(r.parallel_waves.size(), 3u);
    ASSERT_EQ(r.parallel_waves[1].size(), 2u);       // A and B in parallel
}

TEST(cycle_detection) {
    TaskScheduler s;
    s.add_task({"X", "X", 10, 0});
    s.add_task({"Y", "Y", 10, 0});
    s.add_task({"Z", "Z", 10, 0});
    s.add_dependency("X", "Y");
    s.add_dependency("Y", "Z");
    s.add_dependency("Z", "X");
    auto r = s.schedule();
    ASSERT(!r.success);
    ASSERT(!r.cycle.empty());
}

TEST(bottleneck_detection) {
    TaskScheduler s;
    // Only 1 task in each wave → all bottlenecks
    s.add_task({"A", "A", 10, 0});
    s.add_task({"B", "B", 10, 0});
    s.add_dependency("A", "B");
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT_EQ(r.bottlenecks.size(), 2u);
}

TEST(critical_path) {
    TaskScheduler s;
    // A(100) → C(50)   vs   B(10) → C(50)
    s.add_task({"A", "A", 100, 0});
    s.add_task({"B", "B",  10, 0});
    s.add_task({"C", "C",  50, 0});
    s.add_dependency("A", "C");
    s.add_dependency("B", "C");
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT(!r.critical_path.empty());
    // Longest: A(100) + C(50) = 150
    ASSERT_EQ(r.critical_path_duration, 150);
    ASSERT_EQ(r.critical_path[0], "A");
}

TEST(remove_task) {
    TaskScheduler s;
    s.add_task({"A", "A", 10, 0});
    s.add_task({"B", "B", 10, 0});
    s.add_dependency("A", "B");
    ASSERT(s.remove_task("A"));
    ASSERT_EQ(s.task_count(), 1u);
    ASSERT_EQ(s.edge_count(),  0u);
    ASSERT(!s.has_task("A"));
}

TEST(save_load) {
    TaskScheduler s;
    s.add_task({"P", "P", 50, 1});
    s.add_task({"Q", "Q", 80, 0});
    s.add_dependency("P", "Q");
    ASSERT(s.save("dag_test_tmp.txt"));

    TaskScheduler s2;
    ASSERT(s2.load("dag_test_tmp.txt"));
    ASSERT_EQ(s2.task_count(), 2u);
    ASSERT_EQ(s2.edge_count(),  1u);
    auto r = s2.schedule();
    ASSERT(r.success);
}

TEST(demo_build_schedule) {
    TaskScheduler s;
    demos::load_build(s);
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT(r.execution_order.back() == "deploy");
}

TEST(demo_complex_schedule) {
    TaskScheduler s;
    demos::load_complex(s);
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT(r.execution_order[0] == "init");
}

TEST(demo_cycle_fails) {
    TaskScheduler s;
    demos::load_cycle(s);
    auto r = s.schedule();
    ASSERT(!r.success);
}

TEST(priority_ordering) {
    TaskScheduler s;
    // Two independent tasks; higher priority should come first in flat order
    s.add_task({"low",  "low",  10, 0});
    s.add_task({"high", "high", 10, 5});
    auto r = s.schedule();
    ASSERT(r.success);
    ASSERT_EQ(r.execution_order[0], "high");
}

// ─────────────────────────────────────────────
//  Runner
// ─────────────────────────────────────────────
int main() {
    std::cout << "\n\033[1m  DAG Scheduler — Test Suite\033[0m\n"
              << "  ─────────────────────────────────────\n";
    std::cout << "\n  Results: " << tests_passed << " / " << tests_run
              << " passed\n\n";
    return tests_passed == tests_run ? 0 : 1;
}
