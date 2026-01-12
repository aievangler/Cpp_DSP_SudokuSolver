#pragma once
#include <vector>
#include <algorithm>
#include "state.hpp"
#include "geometry.hpp"

struct ShootChoice {
    int cell = -1;
    int digit = -1;
    int score = 0;
    int elim = 0;
    int ns_created = 0;
    int press = 0;
    int mrv = 0;
    int pool = 0;
};

struct ShootScore {
    int score = 0;
    int elim = 0;
    int ns_created = 0;
    int press = 0;
    bool soft_contra = false;
};

struct ShootRule2Weights {
    bool enabled = false;
    int ns = 5;
    int elim = 4;
    int press = 3;
    int mrv2 = 5;
    bool mrv2_only = false;
    int mrv3_pool_gate = 0;
    int mrv3_press_gate = 0;
};

inline int cell_min_pressure(const SolverState& S, int cell, uint16_t mask){
    int row_u = geom::ROW[cell];
    int col_u = 9 + geom::COL[cell];
    int box_u = 18 + geom::BOX[cell];
    int best = 10;
    while(mask){
        int d = __builtin_ctz((unsigned)mask);
        mask &= static_cast<uint16_t>(mask - 1);
        int p = std::min(S.unit_digit_count[row_u][d],
                         std::min(S.unit_digit_count[col_u][d], S.unit_digit_count[box_u][d]));
        if(p < best) best = p;
    }
    return best;
}

inline int shoot_rule_score(int rule, int ns_created, int elim, int press, int mrv,
                            const ShootRule2Weights& rule2){
    if(rule == 2 && rule2.enabled){
        return rule2.ns * ns_created + rule2.elim * elim - rule2.press * press +
               (mrv == 2 ? rule2.mrv2 : 0);
    }
    switch(rule){
        case 1: // NS-heavy
            return 20 * ns_created + 1 * elim - 3 * press + (mrv == 2 ? 5 : 0);
        case 2: // Elim-heavy
            return 5 * ns_created + 4 * elim - 3 * press + (mrv == 2 ? 5 : 0);
        case 3: // Pressure-first
            return 8 * ns_created + 1 * elim - 6 * press + (mrv == 2 ? 5 : 0);
        case 4: // Balanced
            return 12 * ns_created + 3 * elim - 2 * press + (mrv == 2 ? 5 : 0);
        case 0: // Default
        default:
            return 10 * ns_created + 2 * elim - 3 * press + (mrv == 2 ? 5 : 0);
    }
}

inline ShootScore score_shoot_choice(const SolverState& S, const int cell_pop[81], int cell, int d,
                                     int mrv, int rule, const ShootRule2Weights& rule2){
    auto peers = geom::PEER_MASK[cell];
    auto a = geom::band(S.B[d], peers);
    int elim = geom::popcnt(a);
    int ns_created = 0;
    bool soft_contra = false;
    auto tmp = a;
    while(geom::any(tmp)){
        int p = geom::ctz(tmp);
        if(p < 64) tmp.lo &= (tmp.lo - 1);
        else tmp.hi &= (tmp.hi - 1);
        if(cell_pop[p] == 2) ++ns_created;
        if(cell_pop[p] == 1) soft_contra = true;
        int row_u = geom::ROW[p];
        int col_u = 9 + geom::COL[p];
        int box_u = 18 + geom::BOX[p];
        if(S.unit_digit_count[row_u][d] == 1 ||
           S.unit_digit_count[col_u][d] == 1 ||
           S.unit_digit_count[box_u][d] == 1){
            soft_contra = true;
        }
    }
    uint16_t other = static_cast<uint16_t>(S.cell_mask[cell] & ~(1u << d));
    if(other){
        int row_u = geom::ROW[cell];
        int col_u = 9 + geom::COL[cell];
        int box_u = 18 + geom::BOX[cell];
        while(other){
            int d2 = __builtin_ctz((unsigned)other);
            other &= static_cast<uint16_t>(other - 1);
            if(S.unit_digit_count[row_u][d2] == 1 ||
               S.unit_digit_count[col_u][d2] == 1 ||
               S.unit_digit_count[box_u][d2] == 1){
                soft_contra = true;
                break;
            }
        }
    }
    int press = cell_min_pressure(S, cell, static_cast<uint16_t>(1u << d));
    int score = shoot_rule_score(rule, ns_created, elim, press, mrv, rule2);
    return {score, elim, ns_created, press, soft_contra};
}

inline ShootChoice select_shoot_most_ones(const SolverState& S, int pool_cap, bool use_soft, int rule,
                                         bool gate_ns0, const ShootRule2Weights& rule2,
                                         const geom::Bits81* allowed = nullptr){
    ShootChoice choice{};
    int mrv = 10;
    int cell_pop[81];
    for(int c = 0; c < 81; ++c){
        uint16_t mask = S.cell_mask[c];
        cell_pop[c] = popcount9(mask);
        if(S.cell_value[c] != 0) continue;
        if(allowed && !geom::test_bit(*allowed, c)) continue;
        if(cell_pop[c] == 0) {
            choice.mrv = 0;
            return choice;
        }
        if(cell_pop[c] < mrv) mrv = cell_pop[c];
    }
    if(mrv == 10 || mrv <= 1) {
        if(allowed){
            return select_shoot_most_ones(S, pool_cap, use_soft, rule, gate_ns0, rule2, nullptr);
        }
        choice.mrv = mrv == 10 ? 0 : mrv;
        return choice;
    }
    if(rule == 2 && rule2.mrv2_only && mrv != 2){
        choice.mrv = mrv;
        return choice;
    }

    int pool_cells[81];
    int pool_size = 0;
    for(int c = 0; c < 81; ++c){
        if(S.cell_value[c] != 0) continue;
        if(allowed && !geom::test_bit(*allowed, c)) continue;
        if(cell_pop[c] == mrv) pool_cells[pool_size++] = c;
    }

    if(rule == 2 && rule2.mrv3_pool_gate > 0 && rule2.mrv3_press_gate <= 0 &&
       mrv == 3 && pool_size >= rule2.mrv3_pool_gate){
        choice.mrv = mrv;
        choice.pool = pool_size;
        return choice;
    }

    if(pool_size > pool_cap){
        std::vector<std::pair<int,int>> scored;
        scored.reserve(pool_size);
        for(int i = 0; i < pool_size; ++i){
            int c = pool_cells[i];
            int press = cell_min_pressure(S, c, S.cell_mask[c]);
            scored.emplace_back(press, c);
        }
        std::nth_element(scored.begin(),
                         scored.begin() + pool_cap,
                         scored.end(),
                         [](const auto& a, const auto& b){
                             if(a.first != b.first) return a.first < b.first;
                             return a.second < b.second;
                         });
        std::sort(scored.begin(),
                  scored.begin() + pool_cap,
                  [](const auto& a, const auto& b){
                      if(a.first != b.first) return a.first < b.first;
                      return a.second < b.second;
                  });
        pool_size = pool_cap;
        for(int i = 0; i < pool_size; ++i){
            pool_cells[i] = scored[i].second;
        }
    }

    int best_score = -1000000;
    int best_ns = -1;
    int best_elim = -1;
    int best_press = 10;
    int best_cell = -1;
    int best_digit = -1;

    int best_score_any = -1000000;
    int best_ns_any = -1;
    int best_elim_any = -1;
    int best_press_any = 10;
    int best_cell_any = -1;
    int best_digit_any = -1;

    for(int i = 0; i < pool_size; ++i){
        int c = pool_cells[i];
        uint16_t mask = S.cell_mask[c];
        while(mask){
            int d = __builtin_ctz((unsigned)mask);
            mask &= static_cast<uint16_t>(mask - 1);
            auto sc = score_shoot_choice(S, cell_pop, c, d, mrv, rule, rule2);
            bool better_any = false;
            if(sc.score > best_score_any) better_any = true;
            else if(sc.score == best_score_any && sc.ns_created > best_ns_any) better_any = true;
            else if(sc.score == best_score_any && sc.ns_created == best_ns_any && sc.elim > best_elim_any) better_any = true;
            else if(sc.score == best_score_any && sc.ns_created == best_ns_any && sc.elim == best_elim_any && sc.press < best_press_any) better_any = true;
            else if(sc.score == best_score_any && sc.ns_created == best_ns_any && sc.elim == best_elim_any && sc.press == best_press_any && c < best_cell_any) better_any = true;
            else if(sc.score == best_score_any && sc.ns_created == best_ns_any && sc.elim == best_elim_any && sc.press == best_press_any && c == best_cell_any && d < best_digit_any) better_any = true;
            if(better_any){
                best_score_any = sc.score;
                best_ns_any = sc.ns_created;
                best_elim_any = sc.elim;
                best_press_any = sc.press;
                best_cell_any = c;
                best_digit_any = d;
            }

            if(use_soft && sc.soft_contra) continue;

            bool better = false;
            if(sc.score > best_score) better = true;
            else if(sc.score == best_score && sc.ns_created > best_ns) better = true;
            else if(sc.score == best_score && sc.ns_created == best_ns && sc.elim > best_elim) better = true;
            else if(sc.score == best_score && sc.ns_created == best_ns && sc.elim == best_elim && sc.press < best_press) better = true;
            else if(sc.score == best_score && sc.ns_created == best_ns && sc.elim == best_elim && sc.press == best_press && c < best_cell) better = true;
            else if(sc.score == best_score && sc.ns_created == best_ns && sc.elim == best_elim && sc.press == best_press && c == best_cell && d < best_digit) better = true;
            if(better){
                best_score = sc.score;
                best_ns = sc.ns_created;
                best_elim = sc.elim;
                best_press = sc.press;
                best_cell = c;
                best_digit = d;
            }
        }
    }

    if(use_soft && best_cell < 0){
        best_score = best_score_any;
        best_ns = best_ns_any;
        best_elim = best_elim_any;
        best_press = best_press_any;
        best_cell = best_cell_any;
        best_digit = best_digit_any;
    }

    if(rule == 2 && rule2.mrv3_pool_gate > 0 && rule2.mrv3_press_gate > 0 && mrv == 3 &&
       pool_size >= rule2.mrv3_pool_gate && best_press >= rule2.mrv3_press_gate){
        choice.cell = -1;
        choice.digit = -1;
        choice.score = best_score;
        choice.elim = best_elim;
        choice.ns_created = best_ns;
        choice.press = best_press;
        choice.mrv = mrv;
        choice.pool = pool_size;
        return choice;
    }

    if(gate_ns0 && rule == 2 && best_cell >= 0 && mrv == 2 && best_ns == 0){
        choice.cell = -1;
        choice.digit = -1;
        choice.score = best_score;
        choice.elim = best_elim;
        choice.ns_created = best_ns;
        choice.press = best_press;
        choice.mrv = mrv;
        choice.pool = pool_size;
        return choice;
    }

    choice.cell = best_cell;
    choice.digit = best_digit;
    choice.score = best_score;
    choice.elim = best_elim;
    choice.ns_created = best_ns;
    choice.press = best_press;
    choice.mrv = mrv;
    choice.pool = pool_size;
    return choice;
}
