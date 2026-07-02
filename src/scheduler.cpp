#include "scheduler.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <climits>

// ═══════════════════════════════════════════════════════
//  Graph construction
// ═══════════════════════════════════════════════════════

bool TaskScheduler::add_task(const Task& t) {
    if (t.id.empty()) return false;
    if (tasks_.count(t.id)) return false;          // duplicate
    tasks_[t.id] = t;
    adj_[t.id];                                    // ensure entry exists
    radj_[t.id];
    return true;
}

bool TaskScheduler::add_dependency(const std::string& from, const std::string& to) {
    if (!tasks_.count(from) || !tasks_.count(to)) return false;
    if (from == to) return false;                  // self-loop
    // avoid duplicate edges
    auto& vec = adj_[from];
    if (std::find(vec.begin(), vec.end(), to) != vec.end()) return false;
    vec.push_back(to);
    radj_[to].push_back(from);
    ++edge_count_;
    return true;
}

bool TaskScheduler::remove_task(const std::string& id) {
    if (!tasks_.count(id)) return false;
    // Remove all edges involving id
    for (auto& dep : radj_[id]) {
        auto& v = adj_[dep];
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
        --edge_count_;
    }
    for (auto& dep : adj_[id]) {
        auto& v = radj_[dep];
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
        --edge_count_;
    }
    adj_.erase(id);
    radj_.erase(id);
    tasks_.erase(id);
    return true;
}

void TaskScheduler::clear() {
    tasks_.clear();
    adj_.clear();
    radj_.clear();
    edge_count_ = 0;
}

bool TaskScheduler::has_task(const std::string& id) const {
    return tasks_.count(id) > 0;
}

std::vector<std::string> TaskScheduler::dependents_of(const std::string& id) const {
    auto it = adj_.find(id);
    return it != adj_.end() ? it->second : std::vector<std::string>{};
}

std::vector<std::string> TaskScheduler::dependencies_of(const std::string& id) const {
    auto it = radj_.find(id);
    return it != radj_.end() ? it->second : std::vector<std::string>{};
}

// ═══════════════════════════════════════════════════════
//  Cycle detection via DFS (runs before Kahn's)
// ═══════════════════════════════════════════════════════

std::vector<std::string> TaskScheduler::detect_cycle() const {
    enum Color { WHITE, GRAY, BLACK };
    std::unordered_map<std::string, Color> color;
    std::unordered_map<std::string, std::string> parent;
    std::vector<std::string> cycle_path;

    for (auto& kv : tasks_) color[kv.first] = WHITE;

    std::function<bool(const std::string&)> dfs = [&](const std::string& u) -> bool {
        color[u] = GRAY;
        for (const auto& v : adj_.at(u)) {
            if (color[v] == GRAY) {
                // reconstruct cycle
                cycle_path.push_back(v);
                cycle_path.push_back(u);
                std::string cur = u;
                while (cur != v) {
                    cur = parent[cur];
                    cycle_path.push_back(cur);
                }
                std::reverse(cycle_path.begin(), cycle_path.end());
                return true;
            }
            if (color[v] == WHITE) {
                parent[v] = u;
                if (dfs(v)) return true;
            }
        }
        color[u] = BLACK;
        return false;
    };

    for (auto& kv2 : tasks_)
        if (color[kv2.first] == WHITE)
            if (dfs(kv2.first)) return cycle_path;

    return {};
}

// ═══════════════════════════════════════════════════════
//  Kahn's Algorithm — O(V + E)
// ═══════════════════════════════════════════════════════

ScheduleResult TaskScheduler::schedule() const {
    ScheduleResult result;

    // 1. Cycle detection first
    auto cycle = detect_cycle();
    if (!cycle.empty()) {
        result.success = false;
        result.cycle   = cycle;
        return result;
    }

    // 2. Build in-degree map
    std::unordered_map<std::string, int> in_degree;
    for (auto& kv : tasks_) in_degree[kv.first] = 0;
    for (auto& kv : adj_)
        for (auto& v : kv.second)
            in_degree[v]++;

    // 3. Priority queue seeded with zero-in-degree nodes
    //    (higher priority value → earlier execution on tie)
    auto cmp = [&](const std::string& a, const std::string& b) {
        int pa = tasks_.at(a).priority;
        int pb = tasks_.at(b).priority;
        if (pa != pb) return pa < pb;          // higher priority first
        return a > b;                          // lexicographic tiebreak
    };
    std::priority_queue<std::string,
                        std::vector<std::string>,
                        decltype(cmp)> pq(cmp);

    // 4. Compute wave-level (BFS level) for each node
    //    (all tasks at level 0 can start immediately, etc.)
    std::unordered_map<std::string, int> wave_level;
    for (auto& kv : tasks_) wave_level[kv.first] = 0;

    for (auto& kv : in_degree)
        if (kv.second == 0) pq.push(kv.first);

    // First pass to compute wave levels (pure BFS on topology)
    {
        std::unordered_map<std::string, int> tmp_indeg = in_degree;
        std::queue<std::string> bfs;
        for (auto& kv : tmp_indeg)
            if (kv.second == 0) bfs.push(kv.first);
        while (!bfs.empty()) {
            auto u = bfs.front(); bfs.pop();
            for (auto& v : adj_.at(u)) {
                wave_level[v] = std::max(wave_level[v], wave_level[u] + 1);
                if (--tmp_indeg[v] == 0) bfs.push(v);
            }
        }
    }

    // 5. Build parallel waves grouped by level
    int max_wave = 0;
    for (auto& kv : wave_level) max_wave = std::max(max_wave, kv.second);
    result.parallel_waves.resize(max_wave + 1);
    for (auto& kv : wave_level)
        result.parallel_waves[kv.second].push_back(kv.first);

    // Sort each wave by priority (descending), then name
    for (auto& wave : result.parallel_waves) {
        std::sort(wave.begin(), wave.end(), [&](const std::string& a, const std::string& b) {
            int pa = tasks_.at(a).priority;
            int pb = tasks_.at(b).priority;
            if (pa != pb) return pa > pb;
            return a < b;
        });
    }

    // 6. Kahn's main loop → flat execution order
    std::unordered_map<std::string, int> kahn_indeg = in_degree;
    std::priority_queue<std::string,
                        std::vector<std::string>,
                        decltype(cmp)> kahn_pq(cmp);
    for (auto& kv : kahn_indeg)
        if (kv.second == 0) kahn_pq.push(kv.first);

    while (!kahn_pq.empty()) {
        auto u = kahn_pq.top(); kahn_pq.pop();
        result.execution_order.push_back(u);
        for (auto& v : adj_.at(u)) {
            if (--kahn_indeg[v] == 0)
                kahn_pq.push(v);
        }
    }

    // 7. Bottlenecks = waves with exactly one task
    for (auto& wave : result.parallel_waves)
        if (wave.size() == 1)
            result.bottlenecks.push_back(wave[0]);

    // 8. Critical path
    result.critical_path = compute_critical_path(result.parallel_waves);
    for (auto& id : result.critical_path)
        result.critical_path_duration += tasks_.at(id).duration_ms;

    result.success = true;
    return result;
}

// ═══════════════════════════════════════════════════════
//  Critical path (longest weighted path through DAG)
// ═══════════════════════════════════════════════════════

std::vector<std::string> TaskScheduler::compute_critical_path(
    const std::vector<std::vector<std::string>>&) const
{
    // DP: dist[v] = longest cost to reach v (inclusive)
    std::unordered_map<std::string, int>         dist;
    std::unordered_map<std::string, std::string> prev;
    for (auto& kv : tasks_) dist[kv.first] = kv.second.duration_ms;

    // Process in topological order (reuse wave levels for ordering)
    std::unordered_map<std::string, int> wave_level;
    for (auto& kv : tasks_) wave_level[kv.first] = 0;
    {
        std::unordered_map<std::string, int> tmp;
        for (auto& kv : tasks_) tmp[kv.first] = 0;
        for (auto& kv : adj_)
            for (auto& v : kv.second) tmp[v]++;
        std::queue<std::string> q;
        for (auto& kv : tmp) if (kv.second == 0) q.push(kv.first);
        while (!q.empty()) {
            auto u = q.front(); q.pop();
            for (auto& v : adj_.at(u)) {
                wave_level[v] = std::max(wave_level[v], wave_level[u] + 1);
                if (--tmp[v] == 0) q.push(v);
            }
        }
    }

    // Sort all nodes by wave level
    std::vector<std::string> topo;
    topo.reserve(tasks_.size());
    for (auto& kv : tasks_) topo.push_back(kv.first);
    std::sort(topo.begin(), topo.end(),
              [&](const std::string& a, const std::string& b) {
                  return wave_level[a] < wave_level[b];
              });

    for (auto& u : topo) {
        for (auto& v : adj_.at(u)) {
            int candidate = dist[u] + tasks_.at(v).duration_ms;
            if (candidate > dist[v]) {
                dist[v] = candidate;
                prev[v] = u;
            }
        }
    }

    // Find end node with max dist
    std::string end_node;
    int max_dist = INT_MIN;
    for (auto& kv : dist) {
        if (kv.second > max_dist) { max_dist = kv.second; end_node = kv.first; }
    }

    // Backtrack
    std::vector<std::string> path;
    for (std::string cur = end_node; !cur.empty(); ) {
        path.push_back(cur);
        cur = prev.count(cur) ? prev[cur] : "";
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// ═══════════════════════════════════════════════════════
//  Persistence  (simple text format)
// ═══════════════════════════════════════════════════════

bool TaskScheduler::save(const std::string& filepath) const {
    std::ofstream f(filepath);
    if (!f) return false;
    f << "TASKS\n";
    for (auto& kv : tasks_)
        f << kv.first << '\t' << kv.second.name << '\t' << kv.second.duration_ms << '\t' << kv.second.priority << '\n';
    f << "EDGES\n";
    for (auto& kv : adj_)
        for (auto& v : kv.second)
            f << kv.first << '\t' << v << '\n';
    return true;
}

bool TaskScheduler::load(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f) return false;
    clear();
    std::string line, section;
    while (std::getline(f, line)) {
        if (line == "TASKS" || line == "EDGES") { section = line; continue; }
        if (line.empty()) continue;
        std::istringstream ss(line);
        if (section == "TASKS") {
            Task t;
            ss >> t.id >> t.name >> t.duration_ms >> t.priority;
            add_task(t);
        } else if (section == "EDGES") {
            std::string from, to;
            ss >> from >> to;
            add_dependency(from, to);
        }
    }
    return true;
}
