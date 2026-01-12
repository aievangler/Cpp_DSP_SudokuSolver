#pragma once
#include <cstdint>

namespace cfg {
// Enable lightweight runtime checks in debug builds
#ifndef NDEBUG
constexpr bool kDebugChecks = true;
#else
constexpr bool kDebugChecks = false;
#endif

// Queue capacities (upper bounds).
constexpr int kMaxL4Queue = 81;
constexpr int kMaxL1Queue = 27*9;
constexpr int kMaxLockQueue = 27*9;
} // namespace cfg

struct DualConfig {
    bool enabled = false;
    int max_mrv_px = 2;
    int min_depth = 5;
    int max_depth = 8;
    bool require_flat_prop = true;
    int max_py_candidates = 0; // 0 => complete
};

struct SolverConfig {
    DualConfig dual{};
    bool srd_enabled = false;
    int srd_min_depth = 0;
    int srd_max_depth = 99;
    bool shoot_ones = false;
    int shoot_ones_depth = 0;
    bool shoot_ones_soft = false;
    int shoot_ones_rule = 0;
    int shoot_ones_min_score = -1000000;
};
