# Repository Guidelines

## Project Structure & Module Organization
This repository is a C++17 command-line cleaner utility. Source lives in `src/` (core logic, config parsing, logging, and helpers). Configuration defaults are in `configs/` (see `configs/basic.cfg`), and ASCII art assets live in `media/`. Build output is placed in `bin/` by the provided scripts. The build entry point is `CMakeLists.txt`.

## Build, Test, and Development Commands
- `./build.sh` builds on Linux/macOS with CMake, copies the `cleaner` binary to `bin/`, and removes the temporary `build/` directory.
- `build.bat` builds with MinGW on Windows and places `cleaner.exe` in `bin\`.
- `cmake -S . -B build && cmake --build build` is a manual build alternative if you want to keep `build/`.
- Run locally with config and safety flags: `./bin/cleaner --config configs/basic.cfg --dry-run --verbose`.

## Coding Style & Naming Conventions
- Follow the existing C++17 style from `src/`: 4-space indentation, braces on the same line, and `camelCase` for functions/variables.
- Types use `UpperCamelCase`, enums follow `UPPER_UNDERSCORE` values, and logging macros are `SCREAMING_SNAKE_CASE` (e.g., `LOG_INFO`).
- Keep headers paired with `.cpp` (`logger.h`/`logger.cpp`) and use include guards.

## Testing Guidelines
There is no automated test suite in this repo today. If you add tests, place them under a new `tests/` directory, wire them into `CMakeLists.txt`, and document the command you expect contributors to run (e.g., `ctest`).

## Commit & Pull Request Guidelines
Recent history uses short, imperative English subjects (for example, “Handle deletions safely”). Keep commit messages concise and focused. For PRs, include a summary of behavior changes, note any OS-specific impact (Windows/WSL/Linux), and describe how you tested.

## Configuration & Safety Notes
Default config is `configs/basic.cfg`; pass a custom file with `--config`. Prefer `--dry-run` for previews and be explicit when changing deletion behavior or adding new cleanup paths.
