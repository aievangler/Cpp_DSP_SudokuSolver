#include "trail.hpp"
#include "state.hpp"
#include "geometry.hpp"
#include <cassert>

void Trail::undo_to(SolverState& S, size_t to_index){
    while(log.size() > to_index){
<<<<<<< HEAD
        TrailEntry e = log.back(); 
        log.pop_back();

        int c = e.cell;
=======
        auto e = log.back(); log.pop_back();
        int c = e.cell, d = e.digit;
>>>>>>> origin/main
        uint16_t oldm = e.old_mask;
        uint16_t curm = S.cell_mask[c];

        // Restore mask first
        S.cell_mask[c] = oldm;

<<<<<<< HEAD
        const auto& U = geom::CELL_UNITS[c];

        if(e.type == TrailType::ELIM){
            int d = e.digit;

            // Adjust only digit d using old/new mask relationship
            bool had = ((oldm >> d) & 1u) != 0;
            bool has = ((curm >> d) & 1u) != 0; // before undo
            if(had && !has){
                // We had removed d at (c), restore it
                if(c<64) S.B[d].lo |= (1ULL<<c); else S.B[d].hi |= (1ULL<<(c-64));
                for(int ui=0; ui<3; ++ui) ++S.unit_digit_count[U[ui]][d];
            } else if(!had && has){
                // Rare: digit was added (should not happen in normal flow)
                if(c<64) S.B[d].lo &= ~(1ULL<<c); else S.B[d].hi &= ~(1ULL<<(c-64));
                for(int ui=0; ui<3; ++ui) --S.unit_digit_count[U[ui]][d];
            }
        }else{ // PLACE
            // A placement may remove multiple digits from a single cell.
            // Restore B[] and unit counts for ALL digit bit changes between curm and oldm.
            uint16_t removed = (uint16_t)(oldm & (uint16_t)~curm); // bits removed during placement
            uint16_t added   = (uint16_t)(curm & (uint16_t)~oldm); // bits (unexpectedly) added

            while(removed){
                int x = __builtin_ctz((unsigned)removed);
                removed &= (uint16_t)(removed - 1);
                if(c<64) S.B[x].lo |= (1ULL<<c); else S.B[x].hi |= (1ULL<<(c-64));
                for(int ui=0; ui<3; ++ui) ++S.unit_digit_count[U[ui]][x];
            }
            while(added){
                int x = __builtin_ctz((unsigned)added);
                added &= (uint16_t)(added - 1);
                if(c<64) S.B[x].lo &= ~(1ULL<<c); else S.B[x].hi &= ~(1ULL<<(c-64));
                for(int ui=0; ui<3; ++ui) --S.unit_digit_count[U[ui]][x];
            }
=======
        // Adjust B[d] bit for this cell using old/new mask relationship
        auto had_d_before = (oldm >> d) & 1u;
        auto has_d_now    = (curm >> d) & 1u; // before undo
        if(had_d_before && !has_d_now){
            // We had removed d at (c), restore it
            if(c<64) S.B[d].lo |= (1ULL<<c); else S.B[d].hi |= (1ULL<<(c-64));
            // Increment unit counts for the 3 units of c
            const auto& U = geom::CELL_UNITS[c];
            for(int ui=0; ui<3; ++ui) ++S.unit_digit_count[U[ui]][d];
        } else if(!had_d_before && has_d_now){
            // In rare cases, a PLACE removed d from c; old mask did not have d (should be rare)
            if(c<64) S.B[d].lo &= ~(1ULL<<c); else S.B[d].hi &= ~(1ULL<<(c-64));
            const auto& U = geom::CELL_UNITS[c];
            for(int ui=0; ui<3; ++ui) --S.unit_digit_count[U[ui]][d];
        }

        // Restore cell_value if needed
        if(e.type == TrailType::PLACE){
>>>>>>> origin/main
            S.cell_value[c] = 0;
        }
    }
    S.contradiction = false;
    // Any queues/enqueue flags left over belong to the abandoned branch.
    S.q_l4.clear();
    S.q_l1.clear();
    S.q_lock.clear();
    S.enq_l4.fill(0);
    S.enq_l1.fill(0);
    S.enq_lock.fill(0);
}
