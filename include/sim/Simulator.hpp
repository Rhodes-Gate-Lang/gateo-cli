/**
 * File: Simulator.hpp
 * Purpose: Declares register_sim(), which wires the `gate sim` subcommand into
 *          a CLI11 App.
 *
 * Usage (in main):
 *   CLI::App app{"gate"};
 *   gate_cli::sim::register_sim(app);
 *   CLI11_PARSE(app, argc, argv);
 *
 * Adding a new command follows the same pattern — create a register_X(app)
 * function in its own translation unit and call it from main.
 */

#pragma once

#include <CLI/CLI.hpp>

namespace gate_cli::sim {

/// Adds the `sim` subcommand (options, positionals, and callback) to `app`.
void register_sim(CLI::App& app);

}  // namespace gate_cli::sim
