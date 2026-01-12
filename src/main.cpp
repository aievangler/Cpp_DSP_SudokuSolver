#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <chrono>
#include <sys/resource.h>
#include "solver.hpp"

namespace {
using SteadyClock = std::chrono::steady_clock;

double cpu_time_seconds(){
    rusage ru{};
    getrusage(RUSAGE_SELF, &ru);
    double user = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec * 1e-6;
    double sys  = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec * 1e-6;
    return user + sys;
}

inline double wall_ms(SteadyClock::time_point start, SteadyClock::time_point end){
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string trim(const std::string& in){
    size_t start = 0;
    while(start < in.size() && std::isspace(static_cast<unsigned char>(in[start]))) ++start;
    size_t end = in.size();
    while(end > start && std::isspace(static_cast<unsigned char>(in[end-1]))) --end;
    return in.substr(start, end - start);
}

void print_timings(const SolverTimings& solver_times,
                   double solve_wall_ms,
                   double solve_cpu_ms,
                   double output_wall_ms,
                   double output_cpu_ms){
    std::cerr << "timing init(w=" << solver_times.init_wall_ms << "ms cpu=" << solver_times.init_cpu_ms << "ms) "
              << "prop(w=" << solver_times.propagate_wall_ms << "ms cpu=" << solver_times.propagate_cpu_ms << "ms) "
              << "search(w=" << solver_times.search_wall_ms << "ms cpu=" << solver_times.search_cpu_ms << "ms) "
              << "solve_total(w=" << solve_wall_ms << "ms cpu=" << solve_cpu_ms << "ms) "
              << "output(w=" << output_wall_ms << "ms cpu=" << output_cpu_ms << "ms)\n";
}

bool solve_and_print(SudokuSolver& solver, const std::string& puzzle, bool timings_enabled){
    SolverTimings solver_times;
    SolverTimings* timings_ptr = timings_enabled ? &solver_times : nullptr;

    auto solve_wall_start = SteadyClock::now();
    double solve_cpu_start = cpu_time_seconds();
    bool ok = solver.solve(puzzle, timings_ptr);
    auto solve_wall_end = SteadyClock::now();
    double solve_cpu_end = cpu_time_seconds();

    auto output_wall_start = SteadyClock::now();
    double output_cpu_start = cpu_time_seconds();
    if(ok){
        std::cout << solver.solution_string() << "\n";
    }else{
        std::cout << "UNSOLVED/CONTRADICTION\n";
    }
    auto output_wall_end = SteadyClock::now();
    double output_cpu_end = cpu_time_seconds();

    if(timings_enabled){
        print_timings(solver_times,
                      wall_ms(solve_wall_start, solve_wall_end),
                      (solve_cpu_end - solve_cpu_start) * 1000.0,
                      wall_ms(output_wall_start, output_wall_end),
                      (output_cpu_end - output_cpu_start) * 1000.0);
    }

    return ok;
}
} // namespace

int main(int argc, char** argv){
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    bool timings_enabled = false;
    bool dual_enabled = false;
    bool benchmark_mode = false;
    bool srd_enabled = false;
    int srd_min_depth = 0;
    int srd_max_depth = 99;
    bool shoot_ones_enabled = false;
    int shoot_ones_depth = 0;
    bool shoot_ones_soft = false;
    int shoot_ones_rule = 0;
    int shoot_ones_min_score = -1000000;
    std::string file_path;
    std::string dump_times_path;
    std::string puzzle_arg;

    for(int i=1; i<argc; ++i){
        std::string arg = argv[i];
        if(arg == "--file"){
            if(i+1 >= argc){
                std::cerr << "--file requires a path\n";
                return 1;
            }
            file_path = argv[++i];
        }else if(arg == "--timings"){
            timings_enabled = true;
        }else if(arg == "--benchmark"){
            benchmark_mode = true;
        }else if(arg == "--dual-activation"){
            dual_enabled = true;
        }else if(arg == "--srd"){
            srd_enabled = true;
        }else if(arg == "--srd-min-depth"){
            if(i+1 >= argc){
                std::cerr << "--srd-min-depth requires a value\n";
                return 1;
            }
            srd_min_depth = std::stoi(argv[++i]);
        }else if(arg == "--srd-max-depth"){
            if(i+1 >= argc){
                std::cerr << "--srd-max-depth requires a value\n";
                return 1;
            }
            srd_max_depth = std::stoi(argv[++i]);
        }else if(arg == "--shoot-ones"){
            shoot_ones_enabled = true;
        }else if(arg == "--shoot-ones-depth"){
            if(i+1 >= argc){
                std::cerr << "--shoot-ones-depth requires a value\n";
                return 1;
            }
            shoot_ones_enabled = true;
            shoot_ones_depth = std::stoi(argv[++i]);
        }else if(arg == "--shoot-ones-soft"){
            shoot_ones_enabled = true;
            shoot_ones_soft = true;
        }else if(arg == "--shoot-ones-rule"){
            if(i+1 >= argc){
                std::cerr << "--shoot-ones-rule requires a value\n";
                return 1;
            }
            shoot_ones_enabled = true;
            shoot_ones_rule = std::stoi(argv[++i]);
        }else if(arg == "--shoot-ones-min-score"){
            if(i+1 >= argc){
                std::cerr << "--shoot-ones-min-score requires a value\n";
                return 1;
            }
            shoot_ones_enabled = true;
            shoot_ones_min_score = std::stoi(argv[++i]);
        }else if(arg == "--dump-times"){
            if(i+1 >= argc){
                std::cerr << "--dump-times requires a path\n";
                return 1;
            }
            dump_times_path = argv[++i];
        }else{
            puzzle_arg = arg;
        }
    }

    SolverConfig cfg;
    cfg.dual.enabled = dual_enabled;
    cfg.srd_enabled = srd_enabled;
    cfg.srd_min_depth = srd_min_depth;
    cfg.srd_max_depth = srd_max_depth;
    cfg.shoot_ones = shoot_ones_enabled;
    cfg.shoot_ones_depth = shoot_ones_depth;
    cfg.shoot_ones_soft = shoot_ones_soft;
    cfg.shoot_ones_rule = shoot_ones_rule;
    cfg.shoot_ones_min_score = shoot_ones_min_score;

    if(!file_path.empty()){

        std::ifstream in(file_path);
        if(!in){
            std::cerr << "Failed to open " << file_path << "\n";
            return 1;
        }
        SudokuSolver solver(cfg);
        bool all_ok = true;
        std::string line;
        if(benchmark_mode){
            double total_wall_ms = 0.0;
            double total_cpu_ms = 0.0;
            size_t puzzles = 0;
            size_t solved = 0;
            std::ofstream times_out;
            if(!dump_times_path.empty()){
                times_out.open(dump_times_path);
                if(!times_out){
                    std::cerr << "Failed to open " << dump_times_path << "\n";
                    return 1;
                }
                times_out << "index,wall_ms,ok\n";
            }
            while(std::getline(in, line)){
                std::string puzzle = trim(line);
                if(puzzle.empty() || puzzle[0] == '#') continue;
                size_t index = puzzles++;
                auto solve_wall_start = SteadyClock::now();
                double solve_cpu_start = cpu_time_seconds();
                bool ok = solver.solve(puzzle);
                auto solve_wall_end = SteadyClock::now();
                double solve_cpu_end = cpu_time_seconds();
                double solve_wall_ms = wall_ms(solve_wall_start, solve_wall_end);
                total_wall_ms += solve_wall_ms;
                total_cpu_ms += (solve_cpu_end - solve_cpu_start) * 1000.0;
                if(ok) ++solved;
                all_ok = all_ok && ok;
                if(times_out){
                    times_out << index << "," << solve_wall_ms << "," << (ok ? 1 : 0) << "\n";
                }
            }
            std::cout << "benchmark puzzles=" << puzzles
                      << " solved=" << solved
                      << " wall_ms=" << total_wall_ms
                      << " cpu_ms=" << total_cpu_ms << "\n";
        }else{
            while(std::getline(in, line)){
                std::string puzzle = trim(line);
                if(puzzle.empty() || puzzle[0] == '#') continue;
                bool ok = solve_and_print(solver, puzzle, timings_enabled);
                all_ok = all_ok && ok;
            }
        }
        return all_ok ? 0 : 1;
    }

    if(benchmark_mode){
        std::cerr << "--benchmark requires --file\n";
        return 1;
    }

    SudokuSolver solver(cfg);
    std::string puzzle =
        (!puzzle_arg.empty() ? puzzle_arg :
         "53..7...."
         "6..195..."
         ".98....6."
         "8...6...3"
         "4..8.3..1"
         "7...2...6"
         ".6....28."
         "...419..5"
         "....8..79");

    bool ok = solve_and_print(solver, puzzle, timings_enabled);
    return ok ? 0 : 1;
}
