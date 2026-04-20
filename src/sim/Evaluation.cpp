#include "sim/Evaluation.hpp"

#include <cstdint>

namespace gate_cli::sim {

using gateo::v2::view::Node;

namespace {

std::uint64_t mask_width(std::uint32_t width) {
  if (width == 0)
    return 0;
  if (width >= 64)
    return ~std::uint64_t{0};
  return (std::uint64_t{1} << width) - 1;
}

}  // namespace

uint64_t value_of(const Node& node) {
  const std::uint64_t v = node.literal_value.value_or(0);
  return v & mask_width(node.width);
}

void eval(gateo::v2::view::GateObject& obj) {
  using gateo::v2::view::GateType;

  for (Node& node : obj.nodes) {
    switch (node.type) {
      case GateType::Unspecified:
        // TODO: Throw error
        break;

      case GateType::Input:
        // Root inputs keep CLI-bound literal_value. Instance input ports have
        // `inputs[0]` pointing at the driving net in the parent scope.
        if (!node.inputs.empty()) {
          node.literal_value = value_of(obj.nodes[node.inputs.at(0)]);
        }
        break;

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
        std::uint64_t acc = ~0ULL;
        for (std::uint32_t idx : node.inputs) {
          acc &= value_of(obj.nodes[idx]);
        }
        node.literal_value = acc;
        break;
      }

      case GateType::Or: {
        std::uint64_t acc = 0;
        for (std::uint32_t idx : node.inputs) {
          acc |= value_of(obj.nodes[idx]);
        }
        node.literal_value = acc;
        break;
      }

      case GateType::Xor: {
        std::uint64_t acc = 0;
        for (std::uint32_t idx : node.inputs) {
          acc ^= value_of(obj.nodes[idx]);
        }
        node.literal_value = acc;
        break;
      }
    }
  }
}

}  // namespace gate_cli::sim
