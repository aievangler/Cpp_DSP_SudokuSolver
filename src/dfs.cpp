#include "dfs.hpp"
#include "propagation.hpp"
#include "scoring.hpp"
#include "trail.hpp"
#include <algorithm>

<<<<<<< HEAD
namespace {
// Fill cand[0..n) with digits present in a 9-bit mask.
inline int fill_candidates(uint16_t mask, int cand[9]) {
    int n = 0;
    while (mask) {
        int d = __builtin_ctz((unsigned)mask);
        cand[n++] = d;
        mask &= (uint16_t)(mask - 1);
    }
    return n;
}

// Small-n sort (n<=9): descending by score.
template <class ScoreFn>
inline void sort_candidates_desc(int cand[9], int n, ScoreFn score) {
    // Precompute scores once; n<=9 so a tiny selection sort is fastest enough.
    float sc[9];
    for (int i = 0; i < n; ++i) sc[i] = score(cand[i]);
    for (int i = 0; i < n - 1; ++i) {
        int best = i;
        for (int j = i + 1; j < n; ++j) {
            if (sc[j] > sc[best]) best = j;
        }
        if (best != i) {
            std::swap(cand[i], cand[best]);
            std::swap(sc[i], sc[best]);
        }
    }
}
} // namespace

=======
>>>>>>> origin/main
static bool dfs_single_node(SolverState& S, const SolverConfig& cfg, int depth);
static bool dfs_dual_node(SolverState& S, const SolverConfig& cfg, int depth);
static bool dfs_with_py(SolverState& S, const SolverConfig& cfg, int depth, int px);

static inline bool should_use_py(const SolverState& S,
                                 const SolverConfig& cfg,
                                 int depth,
                                 int mrv_px) {
    const DualConfig& dc = cfg.dual;
    if (!dc.enabled) return false;
    if (mrv_px > dc.max_mrv_px) return false;
    if (dc.require_flat_prop && S.last_prop_placements > 0) return false;
    if (depth < dc.min_depth) return false;
    if (depth > dc.max_depth) return false;
    return true;
}

bool dfs_single(SolverState& S, const SolverConfig& cfg) {
    return dfs_single_node(S, cfg, 0);
}

static bool dfs_single_node(SolverState& S, const SolverConfig& cfg, int depth) {
    (void)cfg;
    if (S.is_solved()) return true;

    int c = select_mrv_cell(S);
    if (c < 0) return true;

    uint16_t mask = S.cell_mask[c];
<<<<<<< HEAD
    int cand[9];
    int n = fill_candidates(mask, cand);
    compute_scarcity(S);
    sort_candidates_desc(cand, n, [&](int d){ return score_digit(S, c, d); });

    for (int i = 0; i < n; ++i) {
        int d = cand[i];
=======
    std::vector<int> cand; cand.reserve(9);
    generate_candidates_from_mask(mask, cand);
    compute_scarcity(S);
    std::sort(cand.begin(), cand.end(), [&](int a, int b){
        return score_digit(S, c, a) > score_digit(S, c, b);
    });

    for (int d : cand) {
>>>>>>> origin/main
        size_t mark = S.trail->mark();
        if (!place_digit(S, c, d)) {
            S.trail->undo_to(S, mark);
            continue;
        }
        if (!propagate(S)) {
            S.trail->undo_to(S, mark);
            continue;
        }
        if (dfs_single_node(S, cfg, depth + 1)) {
            return true;
        }
        S.trail->undo_to(S, mark);
    }
    return false;
}

bool dfs_dual(SolverState& S, const SolverConfig& cfg) {
    return dfs_dual_node(S, cfg, 0);
}

static bool dfs_dual_node(SolverState& S, const SolverConfig& cfg, int depth) {
    if (S.is_solved()) return true;

    int px = select_mrv_cell(S);
    if (px < 0) return true;

    uint16_t mask_px = S.cell_mask[px];
    int mrv_px = popcount9(mask_px);

<<<<<<< HEAD
    int cand_x[9];
    int nx = fill_candidates(mask_px, cand_x);
    compute_scarcity(S);
    sort_candidates_desc(cand_x, nx, [&](int d){ return score_digit(S, px, d); });

    for (int ix = 0; ix < nx; ++ix) {
        int dx = cand_x[ix];
=======
    std::vector<int> cand_x; cand_x.reserve(9);
    generate_candidates_from_mask(mask_px, cand_x);
    compute_scarcity(S);
    std::sort(cand_x.begin(), cand_x.end(), [&](int a, int b){
        return score_digit(S, px, a) > score_digit(S, px, b);
    });

    for (int dx : cand_x) {
>>>>>>> origin/main
        size_t mark = S.trail->mark();
        if (!place_digit(S, px, dx)) {
            S.trail->undo_to(S, mark);
            continue;
        }
        if (!propagate(S)) {
            S.trail->undo_to(S, mark);
            continue;
        }

        if (should_use_py(S, cfg, depth, mrv_px)) {
            if (dfs_with_py(S, cfg, depth + 1, px)) {
                return true;
            }
        }

        if (dfs_dual_node(S, cfg, depth + 1)) {
            return true;
        }

        S.trail->undo_to(S, mark);
    }
    return false;
}

static bool dfs_with_py(SolverState& S,
                        const SolverConfig& cfg,
                        int depth,
                        int px) {
    const DualConfig& dc = cfg.dual;
    int py = select_pressure_cell(S, px);
    if (py < 0) return false;

    uint16_t mask_py = S.cell_mask[py];
    if (mask_py == 0) return false;

<<<<<<< HEAD
    int cand_y[9];
    int ny = fill_candidates(mask_py, cand_y);
    compute_scarcity(S);
    sort_candidates_desc(cand_y, ny, [&](int d){ return score_digit(S, py, d); });

    int limit = ny;
=======
    std::vector<int> cand_y; cand_y.reserve(9);
    generate_candidates_from_mask(mask_py, cand_y);
    compute_scarcity(S);
    std::sort(cand_y.begin(), cand_y.end(), [&](int a, int b){
        return score_digit(S, py, a) > score_digit(S, py, b);
    });

    int limit = static_cast<int>(cand_y.size());
>>>>>>> origin/main
    if (dc.max_py_candidates > 0) {
        limit = std::min(limit, dc.max_py_candidates);
    }

    for (int i = 0; i < limit; ++i) {
        int dy = cand_y[i];
        size_t mark2 = S.trail->mark();
        if (!place_digit(S, py, dy)) {
            S.trail->undo_to(S, mark2);
            continue;
        }
        if (!propagate(S)) {
            S.trail->undo_to(S, mark2);
            continue;
        }
        if (dfs_dual_node(S, cfg, depth + 1)) {
            return true;
        }
        S.trail->undo_to(S, mark2);
    }
    return false;
}
