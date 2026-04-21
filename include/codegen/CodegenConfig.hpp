/**
 * File: CodegenConfig.hpp
 * Purpose: Parsed options and loaded design for `gate codegen`.
 *
 * CLI11 binds to the public fields during parsing. Call load_object() from the
 * subcommand callback before any codegen passes read gate_object.
 */

#pragma once

#include "NumberFormatter.hpp"
#include "gateo/v2/view.hpp"

#include <string>

namespace gate_cli::codegen {

struct CodegenConfig {
  // ── Options bound by CLI11 during argument parsing ─────────────────────────

  /// Display / literal formatting convention for generated output (same enum as
  /// `gate sim`; semantics will attach when the emitter is implemented).
  gate_cli::NumberFormat output_format{gate_cli::NumberFormat::Hex};

  /// Directory where generated sources and build artifacts will be written.
  /// Short flag is `-d` (directory), not `-o`, to avoid colliding with the usual
  /// compiler meaning of `-o` (single output file) and with `--output-format`.
  std::string output_dir{"gate-build"};

  /// Path to the `.gateo` file from the positional argument.
  std::string gateo_path;

  // ── Post-parse state ───────────────────────────────────────────────────────

  gateo::v2::view::GateObject gate_object{};

  /// Deserializes gateo_path into gate_object. Call after CLI11 has parsed.
  void load_object();
};

}  // namespace gate_cli::codegen
