#include "sim/Simulator.hpp"

#include "sim/Evaluation.hpp"
#include "sim/SimConfig.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <map>
#include <memory>

namespace gate_cli::sim {

// ── Value maps for CLI11's CheckedTransformer ─────────────────────────────────
//
// CheckedTransformer takes a map<string, T> and:
//   - Validates that the user's string is a key in the map.
//   - Converts the string to the mapped value and stores it in the bound variable.
//   - Automatically builds the "ENUM: {a,b,c}" type hint shown in --help output.
//   - Supports a reverse-lookup so the default value displays as its string form.
//
// Add new aliases by inserting extra entries; no other changes needed.

static const std::map<std::string, InputSource> k_input_source_map{
    {"cli", InputSource::Cli},
};

static const std::map<std::string, NumberFormat> k_number_format_map{
    {"binary",  NumberFormat::Binary},
    {"bin",     NumberFormat::Binary},
    {"dec",     NumberFormat::Dec},
    {"decimal", NumberFormat::Dec},
    {"int",     NumberFormat::Dec},
    {"hex",     NumberFormat::Hex},
};

// ── register_sim ──────────────────────────────────────────────────────────────
//
// Registers the `sim` subcommand on the provided CLI::App.
//
// Pattern for adding a new command (e.g. `gate fmt`):
//   1. Create include/fmt/Formatter.hpp  →  void register_fmt(CLI::App& app);
//   2. Create src/fmt/Formatter.cpp      →  implement register_fmt the same way.
//   3. Call register_fmt(app) in GateCli.cpp.
//   4. Add the .cpp to CMakeLists.txt.

void register_sim(CLI::App& app) {
  // SimConfig owns all parsed values and the loaded design. It is kept alive
  // via shared_ptr so the lambda callback below can capture it safely.
  auto cfg = std::make_shared<SimConfig>();

  CLI::App* sub = app.add_subcommand(
      "sim",
      "Simulate a .gateo design by evaluating its gate network");

  // ── Options ───────────────────────────────────────────────────────────────

  // CLI::ignore_case makes the transformer accept "CLI", "Cli", "cli", etc.
  sub->add_option("--input", cfg->input_source, "Source for input node values")
      ->transform(CLI::CheckedTransformer(k_input_source_map, CLI::ignore_case))
      ->default_str("cli");

  sub->add_option("--output-format", cfg->output_format,
                  "Display format for output node values")
      ->transform(CLI::CheckedTransformer(k_number_format_map, CLI::ignore_case))
      ->default_str("hex");

  // ── Positional arguments ──────────────────────────────────────────────────

  // CLI::ExistingFile validates that the path resolves to a readable file
  // before the callback fires, so load_object() is guaranteed a valid path.
  sub->add_option("file", cfg->gateo_path, "Path to the .gateo design file")
      ->required()
      ->check(CLI::ExistingFile);

  // expected(0, -1) makes the option accept zero or more values, collecting
  // all remaining positional tokens. apply_inputs() splits each "name=value".
  sub->add_option("assignments", cfg->raw_assignments,
                  "Input bindings in name=value form (e.g. A=0b101 B=3)")
      ->expected(0, -1);

  // ── Callback ──────────────────────────────────────────────────────────────
  //
  // CLI11 calls this lambda only when the `sim` subcommand is actually invoked
  // and all required options have been validated. Exceptions thrown here are
  // caught by CLI11_PARSE in main and printed as error messages.

  sub->callback([cfg] {
    cfg->load_object();
    std::cout << "sim: loaded \"" << cfg->gateo_path << "\" — "
              << cfg->gate_object.components.size() << " component(s), "
              << cfg->gate_object.nodes.size() << " node(s)\n";

    cfg->apply_inputs(std::cerr);
    eval(cfg->gate_object);
    cfg->print_outputs(std::cout);
  });
}

}  // namespace gate_cli::sim
