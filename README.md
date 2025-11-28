# C++ Sudoku Solver Built With AI — Fast, Simple, and a Template for Purpose-Specific Solvers

## 1. Overview

This project contains a **fast Sudoku solver written in C++**, designed and implemented with the assistance of AI coding tools. The goal is not just to solve Sudoku, but to demonstrate that **high-performance, purpose-specific solvers can now be built in days**, even by non-programmers.

The solver uses a compact propagation engine and a simple 81×9 bitmask data structure.  
In local tests on an Apple M4 (16GB), the solver runs at:

- **~75,000 puzzles/sec** on the standard 17-clue dataset  
- **top-10-class** relative to published tdoku benchmark results

Reference baseline: https://github.com/t-dillon/tdoku

> **Benchmark Note**  
> • Our results are from local runs on Apple M4.  
> • tdoku results are from Intel Ice Lake (AVX-512).  
> • Speeds across these architectures are not directly comparable.  
> • Ranks shown are approximate and **not yet community-verified**.

Sudoku is used only as a clean, compact constraint system.  
The broader purpose: **to show planners/analysts how quickly AI tools can build custom optimization engines in C++.**

---

## 2. Core Architecture

### Delta-Script Propagation (DSP)
All state changes are handled as **small reversible deltas**:

```
State = BaseRules + Clues + Σ(Deltas)
```

Placing a digit produces a delta-script that:
- fixes the value in the cell  
- eliminates that digit from all 20 peer cells  
- triggers hidden/naked singles (HS/NS)  
- logs each change to a trail for O(1) undo  

This keeps the engine predictable, modular, and easy to extend.

### Bitmask-Based Feasibility (81×9)
Each cell has a **9-bit mask** representing feasible digits:

```
1 = feasible, 0 = eliminated
```

Example:  
`000101110` → digits {4,6,7,8} are feasible.

This 81×9 bitboard is equivalent to the pencilmark diagram: every cell tracks feasibility with 9 binary flags.

### 20-Cell Influence Propagation
Each Sudoku cell has **exactly 20 peers** (row + column + box, deduplicated).  
A delta-script clears the placed digit from these 20 peers via a few bit operations.

The propagation loop uses only:
- **Hidden Singles (HS)**  
- **Naked Singles (NS)**  

No heavy human-style rules (no pairs, chains, X-wings, etc.).

### Engine Loop
```
place → eliminate → propagate → check → undo → continue
```

The solver keeps `S0` (base + initial propagation) fixed, and all DFS search occurs through incremental deltas with reversible undo.

---

## 3. Development Method (AI-Assisted)

This solver was created through a straightforward AI-assisted workflow:

- Logic described in natural language  
- ChatGPT refined the algorithm and designed modules  
- Codex generated and debugged the C++ implementation  
- A second AI performed negative testing and caught edge-case issues  
- Manual and AI testing validated correctness on all benchmark sets

AI handled the engineering; the human provided the logic.  
Testing and verification were done independently.

---

## 4. Performance (Apple M4 16GB)

All puzzle sets 100% solved.

| Dataset | **Our Solver (M4)** | Approx. Rank | tdoku Fastest (Ice Lake AVX-512) |
|--------|-----------------------|--------------|-----------------------------------|
| **17-clue (49,158)** | **~75,000 puzzles/sec** | **~9th** | 364,000 puzzles/sec |
| magictour_top1465 | ~27,700 puzzles/sec | ~11th | 148k–194k puzzles/sec |
| forum_hardest_1106 | ~3,161 puzzles/sec | ~8th–9th | 6,653–13,454 puzzles/sec |
| forum_hardest_1905_11+ | ~6,205 puzzles/sec | ~8th–10th | 6,310–13,454 puzzles/sec |
| Escargot set | ~2,505 puzzles/sec | — | varies widely |

> **Footnote**  
> Our results are architecture-specific local runs.  
> Rankings are puzzles/sec ordering only and **not yet community-verified**.

---

## 5. Why This Matters

Most real-world planning and analysis tasks require **small, purpose-specific solvers** that do not match off-the-shelf tools.

With AI coding tools:

- domain experts can generate solver logic quickly  
- modify or extend it on demand  
- test variations without a long development cycle  
- maintain transparent and explainable engines  

This solver is just a demonstration.  
The same workflow can be applied to build:

- allocation heuristics  
- routing engines  
- inventory/MEIO micro-solvers  
- risk propagation tools  
- custom planning rules  

AI lowers the development barrier dramatically.

---

## 6. Conclusion

This project shows that **a fast, practical solver can be built in days using AI coding tools**, without traditional programming expertise.  
If you have a planning, optimization, or decision process that needs a custom solver, this workflow makes it entirely feasible to build one tailored to your domain.

---

Whether you use this code as a Sudoku solver or as a **template for designing new solvers**, it demonstrates a simple idea:

> **Domain experts can now Imagineer solver logic — and let AI generate fast, clean C++ engines.**
