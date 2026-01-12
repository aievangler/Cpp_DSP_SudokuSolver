#include "dfs.hpp"
#include "propagation.hpp"
#include "scoring.hpp"
#include "shoot_ones.hpp"
#include "trail.hpp"
#include "geometry.hpp"
#include <algorithm>

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

inline int fill_unit_digit_cells(const SolverState& S, int unit, int d, int cells[9]) {
    auto mask = geom::band(S.B[d], geom::UNIT_MASK[unit]);
    int n = 0;
    while (geom::any(mask)) {
        int c = geom::ctz(mask);
        cells[n++] = c;
        if (c < 64) mask.lo &= (mask.lo - 1);
        else mask.hi &= (mask.hi - 1);
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

static int select_srd_unit_digit(const SolverState& S, int mrv, int& best_unit, int& best_digit) {
    int best = mrv;
    best_unit = -1;
    best_digit = -1;
    for (int u = 0; u < 27; ++u) {
        for (int d = 0; d < 9; ++d) {
            int cnt = S.unit_digit_count[u][d];
            if (cnt <= 1 || cnt >= best) continue;
            best = cnt;
            best_unit = u;
            best_digit = d;
            if (best == 2) return best;
        }
    }
    return best_unit >= 0 ? best : 0;
}

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

    int mrv = popcount9(S.cell_mask[c]);
    bool use_shoot = false;
    ShootChoice shoot_choice{};
    ShootRule2Weights rule2{};
    if(cfg.shoot_ones && depth <= cfg.shoot_ones_depth){
        shoot_choice = select_shoot_most_ones(S, 20, cfg.shoot_ones_soft,
                                              cfg.shoot_ones_rule, false, rule2, nullptr);
        if(shoot_choice.cell >= 0 && shoot_choice.digit >= 0 &&
           shoot_choice.score >= cfg.shoot_ones_min_score){
            c = shoot_choice.cell;
            use_shoot = true;
        }
    }

    if (!use_shoot && cfg.srd_enabled && depth >= cfg.srd_min_depth && depth <= cfg.srd_max_depth && mrv > 1) {
        int best_unit = -1;
        int best_digit = -1;
        int best = select_srd_unit_digit(S, mrv, best_unit, best_digit);
        if (best > 0 && best < mrv) {
            int cells[9];
            int n_cells = fill_unit_digit_cells(S, best_unit, best_digit, cells);
            compute_scarcity(S);
            sort_candidates_desc(cells, n_cells, [&](int cell){ return score_digit(S, cell, best_digit); });
            for (int i = 0; i < n_cells; ++i) {
                int cell = cells[i];
                size_t mark = S.trail->mark();
                if (!place_digit(S, cell, best_digit)) {
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
    }

    uint16_t mask = S.cell_mask[c];
    int cand[9];
    int n = fill_candidates(mask, cand);
    compute_scarcity(S);
    if(use_shoot){
        int cell_pop[81];
        for(int i = 0; i < 81; ++i){
            cell_pop[i] = popcount9(S.cell_mask[i]);
        }
        int mrv_c = popcount9(mask);
        int score_for_digit[9];
        bool any_hard = false;
        int raw_score[9];
        for(int i = 0; i < n; ++i){
            int d = cand[i];
            auto sc = score_shoot_choice(S, cell_pop, c, d, mrv_c, cfg.shoot_ones_rule, rule2);
            raw_score[d] = sc.score;
            if(cfg.shoot_ones_soft && sc.soft_contra){
                score_for_digit[d] = -1000000;
            }else{
                score_for_digit[d] = sc.score;
                any_hard = true;
            }
        }
        if(cfg.shoot_ones_soft && !any_hard){
            for(int i = 0; i < n; ++i){
                int d = cand[i];
                score_for_digit[d] = raw_score[d];
            }
        }
        sort_candidates_desc(cand, n, [&](int d){ return (float)score_for_digit[d]; });
    }else{
        sort_candidates_desc(cand, n, [&](int d){ return score_digit(S, c, d); });
    }

    for (int i = 0; i < n; ++i) {
        int d = cand[i];
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

    bool use_shoot = false;
    ShootChoice shoot_choice{};
    ShootRule2Weights rule2{};
    if(cfg.shoot_ones && depth <= cfg.shoot_ones_depth){
        shoot_choice = select_shoot_most_ones(S, 20, cfg.shoot_ones_soft,
                                              cfg.shoot_ones_rule, false, rule2, nullptr);
        if(shoot_choice.cell >= 0 && shoot_choice.digit >= 0 &&
           shoot_choice.score >= cfg.shoot_ones_min_score){
            px = shoot_choice.cell;
            mask_px = S.cell_mask[px];
            mrv_px = popcount9(mask_px);
            use_shoot = true;
        }
    }

    int cand_x[9];
    int nx = fill_candidates(mask_px, cand_x);
    compute_scarcity(S);
    if(use_shoot){
        int cell_pop[81];
        for(int i = 0; i < 81; ++i){
            cell_pop[i] = popcount9(S.cell_mask[i]);
        }
        int mrv_c = popcount9(mask_px);
        int score_for_digit[9];
        bool any_hard = false;
        int raw_score[9];
        for(int i = 0; i < nx; ++i){
            int d = cand_x[i];
            auto sc = score_shoot_choice(S, cell_pop, px, d, mrv_c, cfg.shoot_ones_rule, rule2);
            raw_score[d] = sc.score;
            if(cfg.shoot_ones_soft && sc.soft_contra){
                score_for_digit[d] = -1000000;
            }else{
                score_for_digit[d] = sc.score;
                any_hard = true;
            }
        }
        if(cfg.shoot_ones_soft && !any_hard){
            for(int i = 0; i < nx; ++i){
                int d = cand_x[i];
                score_for_digit[d] = raw_score[d];
            }
        }
        sort_candidates_desc(cand_x, nx, [&](int d){ return (float)score_for_digit[d]; });
    }else{
        sort_candidates_desc(cand_x, nx, [&](int d){ return score_digit(S, px, d); });
    }

    for (int ix = 0; ix < nx; ++ix) {
        int dx = cand_x[ix];
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

    int cand_y[9];
    int ny = fill_candidates(mask_py, cand_y);
    compute_scarcity(S);
    sort_candidates_desc(cand_y, ny, [&](int d){ return score_digit(S, py, d); });

    int limit = ny;
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
