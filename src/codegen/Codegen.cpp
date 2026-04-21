#include "codegen/Codegen.hpp"

#include "codegen/CodegenConfig.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace gate_cli::codegen {

namespace {

// Same aliases as `gate sim --output-format`; keep in sync when extending.
const std::map<std::string, gate_cli::NumberFormat> k_number_format_map{
    {"binary", gate_cli::NumberFormat::Binary},
    {"bin", gate_cli::NumberFormat::Binary},
    {"dec", gate_cli::NumberFormat::Dec},
    {"decimal", gate_cli::NumberFormat::Dec},
    {"int", gate_cli::NumberFormat::Dec},
    {"hex", gate_cli::NumberFormat::Hex},
};

const char* format_label(gate_cli::NumberFormat f) {
  switch (f) {
    case gate_cli::NumberFormat::Binary:
      return "binary";
    case gate_cli::NumberFormat::Hex:
      return "hex";
    case gate_cli::NumberFormat::Dec:
      return "decimal";
    default:
      return "unknown";
  }
}

}  // namespace

void register_codegen(CLI::App& app) {
  auto cfg = std::make_shared<CodegenConfig>();

  CLI::App* sub = app.add_subcommand(
      "codegen",
      "Generate a standalone native binary from a .gateo design (work in progress)");

  sub->add_option("--output-format", cfg->output_format,
                  "Number format used when the emitter prints literals / output values")
      ->transform(CLI::CheckedTransformer(k_number_format_map, CLI::ignore_case))
      ->default_str("hex");

  // `-d` = directory; avoids `-o` (compiler output file) vs format confusion.
  sub->add_option("-d,--output-dir", cfg->output_dir,
                  "Directory for generated sources and build output (created if missing)")
      ->default_str("gate-build");

  sub->add_option("file", cfg->gateo_path, "Path to the .gateo design file")
      ->required()
      ->check(CLI::ExistingFile);

  sub->callback([cfg] {
    cfg->load_object();
    std::cout << "codegen: loaded \"" << cfg->gateo_path << "\" — "
              << cfg->gate_object.components.size() << " component(s), "
              << cfg->gate_object.nodes.size() << " node(s)\n";
    std::cout << "codegen: output directory: \"" << cfg->output_dir << "\"\n";
    std::cout << "codegen: output format: " << format_label(cfg->output_format) << "\n";
    std::cout << "codegen: (emitter not implemented yet)\n";
  });
}

}  // namespace gate_cli::codegen
