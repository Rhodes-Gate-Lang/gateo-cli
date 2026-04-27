#include "sim/SimConfig.hpp"

#include "gateo/io.hpp"

#include <filesystem>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace gate_cli::sim {

void SimConfig::load_object() {
  gate_object = gateo::read_file(std::filesystem::path(gateo_path));
}

void SimConfig::apply_inputs(std::ostream& warn) {
  if (input_source != InputSource::Cli) {
    throw std::logic_error("apply_inputs: only CLI input source is supported");
  }

  using gateo::v3::view::GateType;

  // Index every named root Input node so we can look up assignments by name.
  std::unordered_map<std::string, std::uint32_t> root_input_index;
  for (std::uint32_t n = 0; n < gate_object.nodes.size(); ++n) {
    const auto& node = gate_object.nodes[n];
    if (node.type != GateType::Input || node.parent != 0) {
      continue;
    }
    if (!node.name.has_value() || node.name->empty()) {
      throw std::invalid_argument("root Input node is missing a name (cannot bind CLI inputs)");
    }
    const std::string& nm = *node.name;
    if (root_input_index.count(nm) != 0) {
      throw std::invalid_argument("duplicate root input name: " + nm);
    }
    root_input_index.emplace(nm, n);
  }

  // Process each "name=value" token from raw_assignments.
  std::unordered_set<std::string> satisfied;
  for (const std::string& token : raw_assignments) {
    const std::size_t eq = token.find('=');
    if (eq == std::string::npos) {
      throw std::invalid_argument("expected name=value, got: " + token);
    }

    const std::string name  = token.substr(0, eq);
    const std::string value = token.substr(eq + 1);

    if (name.empty()) {
      throw std::invalid_argument("empty input name in assignment");
    }
    if (!satisfied.insert(name).second) {
      throw std::invalid_argument("duplicate assignment for input: " + name);
    }

    auto it = root_input_index.find(name);
    if (it == root_input_index.end()) {
      throw std::invalid_argument("unknown input name: " + name);
    }

    auto& node = gate_object.nodes[it->second];
    node.value = gate_cli::to_word(value, node.width, &warn);
  }

  // Ensure every named root input received a value.
  for (const auto& entry : root_input_index) {
    if (satisfied.count(entry.first) == 0) {
      throw std::invalid_argument("missing assignment for root input: " + entry.first);
    }
  }
}

void SimConfig::print_outputs(std::ostream& out) const {
  using gateo::v3::view::GateType;

  for (const auto& node : gate_object.nodes) {
    if (node.type != GateType::Output || node.parent != 0 || !node.name.has_value()) {
      continue;
    }

    const std::uint64_t value = node.value.value_or(0);
    const std::string formatted =
        gate_cli::word_to_format(value, output_format, node.width);
    out << *node.name << " [" << node.width << "]: " << formatted << "\n";
  }
}

}  // namespace gate_cli::sim
