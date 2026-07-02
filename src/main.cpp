#include "scheduler.h"
#include "cli.h"
#include "demos.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────
//  Tokenise a single input line
// ─────────────────────────────────────────────
static std::vector<std::string> tokenise(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream ss(line);
    std::string t;
    while (ss >> t) tokens.push_back(t);
    return tokens;
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    using namespace cli::color;

    TaskScheduler sched;
    cli::print_banner();
    cli::print_help();

    // Optional: load a demo or file passed as CLI arg
    if (argc >= 3 && std::string(argv[1]) == "--demo") {
        std::string name = argv[2];
        demos::load(name, sched);
        std::cout << GREEN << "\n  Demo '" << name << "' loaded.\n" << RESET;
        auto r = sched.schedule();
        cli::print_result(r, sched);
        if (argc >= 4 && std::string(argv[3]) == "--exit") return 0;
    }

    std::cout << '\n' << BOLD << "  scheduler> " << RESET;

    std::string line;
    while (std::getline(std::cin, line)) {
        auto tokens = tokenise(line);
        if (tokens.empty()) {
            std::cout << BOLD << "  scheduler> " << RESET;
            continue;
        }

        const std::string& cmd = tokens[0];

        // ── add <id> [name] [duration_ms] [priority] ──
        if (cmd == "add") {
            if (tokens.size() < 2) {
                std::cout << RED << "  Usage: add <id> [name] [duration_ms] [priority]\n" << RESET;
            } else {
                Task t;
                t.id          = tokens[1];
                t.name        = tokens.size() > 2 ? tokens[2] : tokens[1];
                t.duration_ms = tokens.size() > 3 ? std::stoi(tokens[3]) : 100;
                t.priority    = tokens.size() > 4 ? std::stoi(tokens[4]) : 0;
                if (sched.add_task(t))
                    std::cout << GREEN << "  ✔ Task '" << t.id << "' added.\n" << RESET;
                else
                    std::cout << RED << "  ✖ Task '" << t.id << "' already exists.\n" << RESET;
            }
        }

        // ── dep <from> <to> ───────────────────────────
        else if (cmd == "dep") {
            if (tokens.size() < 3) {
                std::cout << RED << "  Usage: dep <from_id> <to_id>\n" << RESET;
            } else {
                if (sched.add_dependency(tokens[1], tokens[2]))
                    std::cout << GREEN << "  ✔ Edge " << tokens[1]
                              << " → " << tokens[2] << " added.\n" << RESET;
                else
                    std::cout << RED << "  ✖ Could not add edge (unknown task or duplicate).\n" << RESET;
            }
        }

        // ── remove <id> ───────────────────────────────
        else if (cmd == "remove" || cmd == "rm") {
            if (tokens.size() < 2) {
                std::cout << RED << "  Usage: remove <id>\n" << RESET;
            } else {
                if (sched.remove_task(tokens[1]))
                    std::cout << YELLOW << "  Removed task '" << tokens[1] << "'.\n" << RESET;
                else
                    std::cout << RED << "  ✖ Task not found.\n" << RESET;
            }
        }

        // ── schedule ──────────────────────────────────
        else if (cmd == "schedule" || cmd == "run" || cmd == "s") {
            if (sched.task_count() == 0) {
                std::cout << YELLOW << "  No tasks. Try: demo basic\n" << RESET;
            } else {
                auto r = sched.schedule();
                cli::print_result(r, sched, true);
            }
        }

        // ── graph ─────────────────────────────────────
        else if (cmd == "graph" || cmd == "g") {
            cli::print_graph(sched);
        }

        // ── demo <name> ───────────────────────────────
        else if (cmd == "demo") {
            std::string name = tokens.size() > 1 ? tokens[1] : "basic";
            demos::load(name, sched);
            std::cout << GREEN << "  ✔ Demo '" << name << "' loaded ("
                      << sched.task_count() << " tasks, "
                      << sched.edge_count()  << " edges). Run: schedule\n" << RESET;
        }

        // ── save <file> ───────────────────────────────
        else if (cmd == "save") {
            std::string path = tokens.size() > 1 ? tokens[1] : "graph.dag";
            if (sched.save(path))
                std::cout << GREEN << "  ✔ Saved to " << path << '\n' << RESET;
            else
                std::cout << RED << "  ✖ Could not write file.\n" << RESET;
        }

        // ── load <file> ───────────────────────────────
        else if (cmd == "load") {
            if (tokens.size() < 2) {
                std::cout << RED << "  Usage: load <file>\n" << RESET;
            } else {
                if (sched.load(tokens[1]))
                    std::cout << GREEN << "  ✔ Loaded " << sched.task_count()
                              << " tasks from " << tokens[1] << '\n' << RESET;
                else
                    std::cout << RED << "  ✖ Could not read file.\n" << RESET;
            }
        }

        // ── clear ─────────────────────────────────────
        else if (cmd == "clear" || cmd == "reset") {
            sched.clear();
            std::cout << YELLOW << "  Graph cleared.\n" << RESET;
        }

        // ── help ──────────────────────────────────────
        else if (cmd == "help" || cmd == "h" || cmd == "?") {
            cli::print_help();
        }

        // ── exit ──────────────────────────────────────
        else if (cmd == "exit" || cmd == "quit" || cmd == "q") {
            std::cout << CYAN << "  Goodbye.\n" << RESET;
            break;
        }

        else {
            std::cout << RED << "  Unknown command: " << cmd
                      << ".  Type 'help'.\n" << RESET;
        }

        std::cout << '\n' << BOLD << "  scheduler> " << RESET;
    }

    return 0;
}
