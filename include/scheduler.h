#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>

// ─────────────────────────────────────────────
//  Task descriptor
// ─────────────────────────────────────────────
struct Task {
    std::string id;
    std::string name;
    int         duration_ms   = 1;   // simulated execution time
    int         priority      = 0;   // tiebreak (higher = sooner)

    bool operator==(const Task& o) const { return id == o.id; }
};

// ─────────────────────────────────────────────
//  Result of a schedule() call
// ─────────────────────────────────────────────
struct ScheduleResult {
    bool success = false;

    // Flat topological order (all tasks)
    std::vector<std::string> execution_order;

    // Parallel waves: tasks[i] can all run simultaneously
    std::vector<std::vector<std::string>> parallel_waves;

    // Bottleneck stages: tasks that are the sole item in a wave
    std::vector<std::string> bottlenecks;

    // If !success, the cycle is reported here
    std::vector<std::string> cycle;

    // Critical path (longest dependency chain)
    std::vector<std::string> critical_path;
    int critical_path_duration = 0;
};

// ─────────────────────────────────────────────
//  DAG-based Task Scheduler
// ─────────────────────────────────────────────
class TaskScheduler {
public:
    // ── Graph construction ──────────────────
    bool add_task(const Task& t);
    bool add_dependency(const std::string& from,   // must finish first
                        const std::string& to);     // depends on 'from'
    bool remove_task(const std::string& id);
    void clear();

    // ── Core algorithm ──────────────────────
    // Runs Kahn's + cycle detection + wave computation
    ScheduleResult schedule() const;

    // ── Queries ─────────────────────────────
    bool        has_task(const std::string& id) const;
    std::vector<std::string> dependents_of(const std::string& id) const;
    std::vector<std::string> dependencies_of(const std::string& id) const;
    size_t      task_count()  const { return tasks_.size(); }
    size_t      edge_count()  const { return edge_count_; }

    // ── Persistence ─────────────────────────
    bool save(const std::string& filepath) const;
    bool load(const std::string& filepath);

private:
    std::unordered_map<std::string, Task>                   tasks_;
    std::unordered_map<std::string, std::vector<std::string>> adj_;   // id → dependents
    std::unordered_map<std::string, std::vector<std::string>> radj_;  // id → dependencies
    size_t edge_count_ = 0;

    // Helpers
    std::vector<std::string> detect_cycle() const;
    std::vector<std::string> compute_critical_path(
        const std::vector<std::vector<std::string>>& waves) const;
};
