/**
 * File: GateCli.cpp
 * Purpose: Entry point for the `gate` CLI.
 *
 * To add a new top-level command (e.g. `gate fmt`):
 *   1. Implement register_fmt(CLI::App&) in its own translation unit.
 *   2. Include its header below.
 *   3. Call register_fmt(app) alongside the other register_* calls.
 *   4. Add the new .cpp file to gate_cli in CMakeLists.txt.
 *
 * Commands: sim, codegen, …
 */

#include "codegen/Codegen.hpp"
#include "sim/Simulator.hpp"

#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
  CLI::App app{"Gate-Lang CLI — tooling for the Gate-Lang HDL"};

  // Require exactly one subcommand per invocation. CLI11 automatically prints
  // available subcommands and usage when none is provided.
  app.require_subcommand(1);

  // --help-all expands subcommand help inline, useful for quick reference.
  app.set_help_all_flag("--help-all", "Show help for all subcommands");

  // ── Register subcommands ──────────────────────────────────────────────────
  gate_cli::codegen::register_codegen(app);
  gate_cli::sim::register_sim(app);

  // app.parse() is split out here (rather than using the CLI11_PARSE macro)
  // so we can also catch std::exception thrown from subcommand callbacks.
  // CLI::ParseError covers bad flags, missing required args, etc. and is
  // handled by app.exit(), which prints the error + relevant usage string.
  // Any other exception (e.g. domain errors from load_object/apply_inputs)
  // falls through to the second catch and is printed as a plain error.
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
