/**
 * File: SimConfig.hpp
 * Purpose: Hold the parsed options and loaded design for a `gate sim` run.
 *
 * CLI11 binds directly to the public fields during argument parsing.
 * Call load_object() then apply_inputs() in the subcommand callback
 * before handing gate_object off to eval().
 */

#pragma once

#include "NumberFormatter.hpp"
#include "gateo/v2/view.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace gate_cli::sim {

enum class InputSource { Cli };

struct SimConfig {
  // ── Options bound by CLI11 during argument parsing ──────────────────────────

  InputSource input_source{InputSource::Cli};
  gate_cli::NumberFormat output_format{gate_cli::NumberFormat::Hex};

  /// Filesystem path to the `.gateo` file supplied as a positional argument.
  std::string gateo_path;

  /// Raw `name=value` tokens from the positional `[assignments...]` argument.
  /// Parsed into node bindings by apply_inputs().
  std::vector<std::string> raw_assignments;

  // ── Post-parse state ─────────────────────────────────────────────────────────

  /// Loaded design; mutable so eval() and future passes can update node values.
  gateo::v2::view::GateObject gate_object{};

  // ── Methods called from the CLI11 subcommand callback ────────────────────────

  /// Reads gateo_path into gate_object. Must be called after CLI11 has parsed.
  void load_object();

  /// Binds raw_assignments onto root (parent == 0) Input nodes.
  /// Truncation warnings go to `warn`. Throws std::invalid_argument if names
  /// are unknown, assignments are malformed, or required inputs are missing.
  void apply_inputs(std::ostream& warn);

  /// Prints each named root Output node to `out` after eval() has run.
  /// Values are formatted using output_format and zero-padded to node width.
  void print_outputs(std::ostream& out) const;
};

}  // namespace gate_cli::sim
