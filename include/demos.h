#pragma once
#include "scheduler.h"
#include <string>

// ─────────────────────────────────────────────
//  Pre-built demo scenarios
// ─────────────────────────────────────────────
namespace demos {

// ── 1. Basic 4-task linear chain ─────────────
inline void load_basic(TaskScheduler& s) {
    s.clear();
    s.add_task({"A", "Fetch Data",    200, 2});
    s.add_task({"B", "Parse Data",    150, 2});
    s.add_task({"C", "Validate",      100, 1});
    s.add_task({"D", "Store Results", 300, 0});
    s.add_dependency("A", "B");
    s.add_dependency("B", "C");
    s.add_dependency("C", "D");
}

// ── 2. Complex multi-branch DAG ──────────────
inline void load_complex(TaskScheduler& s) {
    s.clear();
    // Tasks
    s.add_task({"init",    "Initialise",      50,  3});
    s.add_task({"fetch1",  "Fetch Source 1",  200, 2});
    s.add_task({"fetch2",  "Fetch Source 2",  180, 2});
    s.add_task({"parse1",  "Parse Source 1",  120, 1});
    s.add_task({"parse2",  "Parse Source 2",  130, 1});
    s.add_task({"merge",   "Merge Results",   100, 2});
    s.add_task({"enrich",  "Enrich Data",     250, 1});
    s.add_task({"validate","Validate",         80, 2});
    s.add_task({"store",   "Store to DB",     300, 0});
    s.add_task({"notify",  "Send Notify",      40, 0});
    // Edges
    s.add_dependency("init",   "fetch1");
    s.add_dependency("init",   "fetch2");
    s.add_dependency("fetch1", "parse1");
    s.add_dependency("fetch2", "parse2");
    s.add_dependency("parse1", "merge");
    s.add_dependency("parse2", "merge");
    s.add_dependency("merge",  "enrich");
    s.add_dependency("enrich", "validate");
    s.add_dependency("validate","store");
    s.add_dependency("store",  "notify");
}

// ── 3. Cycle demo (should fail) ──────────────
inline void load_cycle(TaskScheduler& s) {
    s.clear();
    s.add_task({"X", "Task X", 100, 0});
    s.add_task({"Y", "Task Y", 100, 0});
    s.add_task({"Z", "Task Z", 100, 0});
    s.add_dependency("X", "Y");
    s.add_dependency("Y", "Z");
    s.add_dependency("Z", "X");   // ← cycle!
}

// ── 4. Realistic C++ build-system pipeline ───
inline void load_build(TaskScheduler& s) {
    s.clear();
    // Sources
    s.add_task({"src_main",  "main.cpp",       10, 3});
    s.add_task({"src_util",  "util.cpp",       10, 3});
    s.add_task({"src_net",   "net.cpp",        10, 3});
    s.add_task({"src_db",    "db.cpp",         10, 3});
    // Compile
    s.add_task({"cc_main",   "compile main",   80, 2});
    s.add_task({"cc_util",   "compile util",   60, 2});
    s.add_task({"cc_net",    "compile net",    70, 2});
    s.add_task({"cc_db",     "compile db",     75, 2});
    // Link steps
    s.add_task({"link_app",  "link app",      120, 1});
    s.add_task({"link_test", "link tests",     90, 1});
    // Test + package
    s.add_task({"run_tests", "run unit tests",200, 1});
    s.add_task({"package",   "package binary", 50, 0});
    s.add_task({"deploy",    "deploy",        150, 0});

    // Compile deps on source
    s.add_dependency("src_main",  "cc_main");
    s.add_dependency("src_util",  "cc_util");
    s.add_dependency("src_net",   "cc_net");
    s.add_dependency("src_db",    "cc_db");
    // Link depends on compile
    s.add_dependency("cc_main",   "link_app");
    s.add_dependency("cc_util",   "link_app");
    s.add_dependency("cc_net",    "link_app");
    s.add_dependency("cc_db",     "link_app");
    s.add_dependency("cc_util",   "link_test");
    // Test and package
    s.add_dependency("link_app",  "run_tests");
    s.add_dependency("link_test", "run_tests");
    s.add_dependency("run_tests", "package");
    s.add_dependency("package",   "deploy");
}

inline void load(const std::string& name, TaskScheduler& s) {
    if      (name == "basic")   load_basic(s);
    else if (name == "complex") load_complex(s);
    else if (name == "cycle")   load_cycle(s);
    else if (name == "build")   load_build(s);
}

} // namespace demos
