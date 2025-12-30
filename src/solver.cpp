#include "solver.hpp"
#include "geometry.hpp"
#include "propagation.hpp"
#include "dfs.hpp"
#include <sstream>
#include <chrono>
#include <sys/resource.h>

namespace {
using SteadyClock = std::chrono::steady_clock;
inline double cpu_time_seconds(){
    rusage ru{};
    getrusage(RUSAGE_SELF, &ru);
    double user = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec * 1e-6;
    double sys  = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec * 1e-6;
    return user + sys;
}
inline double wall_ms(SteadyClock::time_point start, SteadyClock::time_point end){
    return std::chrono::duration<double, std::milli>(end - start).count();
}
<<<<<<< HEAD

bool validate_solution(const SolverState& S, const std::string& puzzle){
    // Check all cells filled and fixed clues honored.
    for(int i = 0; i < 81; ++i){
        int v = S.cell_value[i];
        if(v < 1 || v > 9) return false;
        if(i < (int)puzzle.size()){
            char ch = puzzle[i];
            if(ch >= '1' && ch <= '9' && v != (ch - '0')) return false;
        }
    }

    // Rows
    for(int r = 0; r < 9; ++r){
        int mask = 0;
        for(int c = 0; c < 9; ++c){
            int v = S.cell_value[r * 9 + c] - 1;
            int bit = 1 << v;
            if(mask & bit) return false;
            mask |= bit;
        }
        if(mask != 0x1FF) return false;
    }

    // Cols
    for(int c = 0; c < 9; ++c){
        int mask = 0;
        for(int r = 0; r < 9; ++r){
            int v = S.cell_value[r * 9 + c] - 1;
            int bit = 1 << v;
            if(mask & bit) return false;
            mask |= bit;
        }
        if(mask != 0x1FF) return false;
    }

    // Boxes
    for(int br = 0; br < 3; ++br){
        for(int bc = 0; bc < 3; ++bc){
            int mask = 0;
            for(int r = 0; r < 3; ++r){
                for(int c = 0; c < 3; ++c){
                    int idx = (br * 3 + r) * 9 + (bc * 3 + c);
                    int v = S.cell_value[idx] - 1;
                    int bit = 1 << v;
                    if(mask & bit) return false;
                    mask |= bit;
                }
            }
            if(mask != 0x1FF) return false;
        }
    }

    return true;
}
=======
>>>>>>> origin/main
} // namespace

SudokuSolver::SudokuSolver(const SolverConfig& cfg) : config_(cfg) {
    static bool geom_inited = false;
    if(!geom_inited){
        geom::init();
        geom_inited = true;
    }
    S_.trail = &trail_;
    trail_.reserve(1<<16);
}

bool SudokuSolver::solve(const std::string& puzzle, SolverTimings* timings){
<<<<<<< HEAD
    // Important: this solver instance can be reused across many puzzles (benchmark mode).
    // The trail must be cleared per puzzle; otherwise memory grows without bound.
    trail_.log.clear();

=======
>>>>>>> origin/main
    auto init_wall_start = SteadyClock::now();
    double init_cpu_start = cpu_time_seconds();
    S_.init_from_puzzle(puzzle);
    auto init_wall_end = SteadyClock::now();
    double init_cpu_end = cpu_time_seconds();
    if(timings){
        timings->init_wall_ms = wall_ms(init_wall_start, init_wall_end);
        timings->init_cpu_ms = (init_cpu_end - init_cpu_start) * 1000.0;
    }

    // Initial propagation: for any pre-fixed cells, queues were primed
    auto prop_wall_start = SteadyClock::now();
    double prop_cpu_start = cpu_time_seconds();
    bool ok = propagate(S_);
    auto prop_wall_end = SteadyClock::now();
    double prop_cpu_end = cpu_time_seconds();
    if(timings){
        timings->propagate_wall_ms = wall_ms(prop_wall_start, prop_wall_end);
        timings->propagate_cpu_ms = (prop_cpu_end - prop_cpu_start) * 1000.0;
    }
    if(!ok) return false;

    auto search_wall_start = SteadyClock::now();
    double search_cpu_start = cpu_time_seconds();
    if(config_.dual.enabled){
        ok = dfs_dual(S_, config_);
    }else{
        ok = dfs_single(S_, config_);
    }
    auto search_wall_end = SteadyClock::now();
    double search_cpu_end = cpu_time_seconds();
    if(timings){
        timings->search_wall_ms = wall_ms(search_wall_start, search_wall_end);
        timings->search_cpu_ms = (search_cpu_end - search_cpu_start) * 1000.0;
    }

<<<<<<< HEAD
    if(ok && !validate_solution(S_, puzzle)) return false;
=======
>>>>>>> origin/main
    return ok;
}

std::string SudokuSolver::solution_string() const{
    std::ostringstream oss;
    for(int i=0;i<81;++i){
        int v = S_.cell_value[i];
        if(v==0){
            // produce a best-effort candidate (optional)
            oss << '0';
        }else{
            oss << char('0' + v);
        }
    }
    return oss.str();
}
