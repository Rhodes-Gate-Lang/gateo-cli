#include "sim/Evaluation.hpp"

#include <cstdint>

namespace gate_cli::sim {

using gateo::v2::view::Node;

uint64_t value_of(const Node& node) {
  return node.literal_value.value_or(0);
}

void eval(gateo::v2::view::GateObject& obj) {
  using gateo::v2::view::GateType;

  for (Node& node : obj.nodes) {
    switch (node.type) {
      case GateType::Unspecified:
        // TODO: Throw error
        break;

      case GateType::Input:
        break; // Do nothing

      case GateType::Literal:
        break; // Value is already set

      case GateType::Output: {
        uint64_t v = value_of(obj.nodes[node.inputs.at(0)]);
        node.literal_value = v;
        break;
      }

      case GateType::Not: {
        uint64_t v = value_of(obj.nodes[node.inputs.at(0)]);
        node.literal_value = (~v);
        break;
      }

      case GateType::And: {
        node.literal_value = ~0ULL;
        for (uint32_t idx : node.inputs)
          node.literal_value &= value_of(obj.nodes[idx]);

        break;
      }

      case GateType::Or: {
        node.literal_value = 0;
        for (uint32_t idx : node.inputs)
          node.literal_value |= value_of(obj.nodes[idx]);

        break;
      }

      case GateType::Xor: {
        node.literal_value = 0;
        for (uint32_t idx : node.inputs)
          node.literal_value ^= value_of(obj.nodes[idx]);

        break;
      }
    }
  }
}

}  // namespace gate_cli::sim
