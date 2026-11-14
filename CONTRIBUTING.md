# Contributing

Use Windows x64, Python 3.10+, CMake 3.20+, Ninja, and a C++17 toolchain.
MSVC and MinGW are supported by the build configuration; local validation uses MinGW.

1. Install development tools with `uv sync --group dev` (or install the pinned
   packages from `pyproject.toml` in a virtual environment).
2. Edit `tools/generate_themes.py` and regenerate skins with
   `python tools/generate_themes.py`. Do not hand-edit generated `.ini` files.
3. Run `ruff check .`, `ruff format --check .`, and
   `python tools/check.py`. Format C++ with `clang-format -i src/*.cpp tests/*.cpp`.
4. Configure and build as shown in the README. Warnings are errors.
5. Run `python -m unittest discover -s tests -v` and
   `python tools/smoke_telemetry.py --dll build/PhoneTelemetry.dll`.

Hardware and ETW checks need a real Windows machine. Never present a synthetic
renderer result as evidence that a particular game or anti-cheat is supported.
Do not commit binaries, personal Rainmeter layouts, credentials, or runtime state.

Contributions are provided under the repository's MIT license.
