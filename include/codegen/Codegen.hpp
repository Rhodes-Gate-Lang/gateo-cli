/**
 * File: Codegen.hpp
 * Purpose: Declares register_codegen(), which wires `gate codegen` into CLI11.
 */

#pragma once

#include <CLI/CLI.hpp>

namespace gate_cli::codegen {

/// Adds the `codegen` subcommand (options, positionals, callback) to `app`.
void register_codegen(CLI::App& app);

}  // namespace gate_cli::codegen
