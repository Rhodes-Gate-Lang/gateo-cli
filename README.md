# gateo-cli

Command-line tool for working with compiled Gate-Lang `.gateo` artifacts (gateo.v2 wire format).

## Architecture

**Command pattern** — Each subcommand is a `Command` subclass: it implements `run(std::vector<std::string> args)` and `get_info()` (name, description, usage). New commands add a `.cpp` / header pair and stay isolated from parsing and dispatch.

**Registry pattern** — `Registry` owns a map from command name → `std::unique_ptr<Command>`. The first CLI token selects the command; remaining tokens are passed as `args`. Built-ins like `help` are handled in the registry (e.g. `help` prints every registered command’s `CommandInfo`).

**Entry point** — `src/cli/GateCli.cpp` turns `argv` into a `std::vector<std::string>`, splits the command name from the tail, and forwards to `Registry::run_command`.

## Dependency: gateo-cpp

This repo does **not** vendor the parser/runtime by hand. [gateo-cpp](https://github.com/Rhodes-Gate-Lang/gateo-cpp) is pulled in at configure time via CMake `FetchContent` (pinned tag in `CMakeLists.txt`). The static library `gateo::gateo` provides I/O and the native view type `gateo::v2::view::GateObject` (e.g. `gateo::read_file` for `.gateo` files). Commands such as `sim` link against `gate_cli`, which links `gateo::gateo` publicly so protobuf and generated code resolve for the final `gate` binary.

## Build

Requires CMake ≥ 3.20, a C++17 compiler, and **protobuf** (C++ dev package), or configure gateo-cpp with protobuf fetch options if you prefer a source build.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is `build/gate` (CMake target `gate-lang-cli`).

## Usage

```text
gate <command> [arguments...]
gate help
gate sim path/to/design.gateo
```

`sim` loads the file through gateo-cpp and runs whatever simulation/inspection logic that command implements.
