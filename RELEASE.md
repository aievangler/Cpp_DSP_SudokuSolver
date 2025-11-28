# Publishing the C++ Solver

This directory (`cppsolver_repo/`) is structured as a drop-in Git repository. Use this checklist whenever you want to
publish or refresh the standalone C++ solver.

## 1. Pre-Publish Checklist

1. Verify a clean source tree (no `build/` folder or other artifacts).
2. Configure + build:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ```
3. Run the CLI once with the embedded sample file and any internal benchmark set:
   ```bash
   ./build/cppsolver --file puzzles/sample_hard95.txt --benchmark
   ./build/cppsolver "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79"
   ```
4. Update `README.md` with any new performance numbers or feature toggles discovered during testing.
5. Remove the `build/` directory before committing (`rm -rf build`).

## 2. Create the Standalone Repository

```bash
cd /path/to/SudokuSolver_20251104/cppsolver_repo
rm -rf build
rm -rf .git  # if left from a prior standalone init
git init
git add .
git commit -m "Initial release: event-driven C++ Sudoku solver"
```

## 3. Publish on GitHub (or any remote)

1. Create a new empty repository in the desired organization/user account (e.g. `aievangler/cppsolver`).
2. Add it as a remote and push:
   ```bash
   git remote add origin git@github.com:aievangler/cppsolver.git
   git branch -M main
   git push -u origin main
   ```
3. Add repository topics (`sudoku`, `cpp`, `solver`, `constraint-programming`, etc.)
4. Create a release tag once validated, e.g. `git tag v0.1.0 && git push origin v0.1.0`.

## 4. Publish Documentation / New Page

- The `README.md` is written to serve as the GitHub landing page. Copy its sections directly into your website or
  internal wiki to create the “new page” announcing the release.
- Include the `puzzles/sample_hard95.txt` snippet or link to your benchmark datasets so visitors can reproduce
your numbers.
- Embed the CLI snippets and `Embedding in Other Projects` section where developers expect API documentation.
- Add screenshots / timing tables if you have visuals from the Python project.

## 5. Reuse Inside the Python Monorepo

After pushing the standalone repo you can still consume it here via one of two paths:

- Drop the remote repo somewhere under `thirdparty/` and `add_subdirectory` it from any C++ tooling.
- Or keep using the in-tree copy for local development and periodically `rsync` changes out to the standalone repo.

Keeping this file up-to-date ensures future cutovers stay quick and reproducible.
