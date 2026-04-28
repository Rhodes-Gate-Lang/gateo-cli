#include "sim/Evaluation.hpp"

#include <cstdint>

namespace gate_cli::sim {

using gateo::v3::view::Node;

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
  const std::uint64_t v = node.value.value_or(0);
  return v & mask_width(node.width);
}

void eval(gateo::v3::view::GateObject& obj) {
  using gateo::v3::view::GateType;

  for (Node& node : obj.nodes) {
    switch (node.type) {
      case GateType::Unspecified:
        // TODO: Throw error
        break;

      case GateType::Input:
        // Root inputs keep CLI-bound value. Instance input ports have
        // `inputs[0]` pointing at the driving net in the parent scope.
        if (!node.inputs.empty()) {
          node.value = value_of(obj.nodes[node.inputs.at(0)]);
        }
        break;

      case GateType::Literal:
        break; // Value is already set

      case GateType::Output: {
        uint64_t v = value_of(obj.nodes[node.inputs.at(0)]);
        node.value = v;
        break;
      }

      case GateType::Not: {
        uint64_t v = value_of(obj.nodes[node.inputs.at(0)]);
        node.value = (~v);
        break;
      }

      case GateType::And: {
        std::uint64_t acc = ~0ULL;
        for (std::uint32_t idx : node.inputs) {
          acc &= value_of(obj.nodes[idx]);
        }
        node.value = acc;
        break;
      }

      case GateType::Or: {
        std::uint64_t acc = 0;
        for (std::uint32_t idx : node.inputs) {
          acc |= value_of(obj.nodes[idx]);
        }
        node.value = acc;
        break;
      }

      case GateType::Xor: {
        std::uint64_t acc = 0;
        for (std::uint32_t idx : node.inputs) {
          acc ^= value_of(obj.nodes[idx]);
        }
        node.value = acc;
        break;
      }

      case GateType::Split: {
        const Node& src_node = obj.nodes[node.inputs.at(0)];
        const std::uint32_t src_w = src_node.width;
        const std::uint64_t v = value_of(src_node);
        const std::uint32_t split_lo = node.split_lo.value_or(0);
        std::uint64_t out = 0;
        for (std::uint32_t b = 0; b < node.width; ++b) {
          // Output LSB index b comes from parent MSB index split_lo + (width - 1 - b).
          const std::uint32_t src_msb_idx = split_lo + (node.width - 1 - b);
          const int lsb_in_parent =
              static_cast<int>(src_w - 1) - static_cast<int>(src_msb_idx);
          const std::uint64_t bit =
              (lsb_in_parent >= 0 && lsb_in_parent < 64) ? ((v >> lsb_in_parent) & 1) : 0;
          out |= bit << b;
        }
        node.value = out & mask_width(node.width);
        break;
      }

      case GateType::Merge: {
        std::uint64_t out = 0;
        for (std::uint32_t idx : node.inputs) {
          const Node& in_node = obj.nodes[idx];
          const std::uint32_t w = in_node.width;
          out = (out << w) | (value_of(in_node) & mask_width(w));
        }
        node.value = out & mask_width(node.width);
        break;
      }

      case GateType::Lsl: {
        const std::uint64_t x = value_of(obj.nodes[node.inputs.at(0)]);
        const std::uint64_t sh = value_of(obj.nodes[node.inputs.at(1)]);
        const std::uint32_t w = node.width;
        std::uint64_t out = 0;
        if (sh < 64)
          out = (x << sh) & mask_width(w);
        node.value = out;
        break;
      }

      case GateType::Lsr: {
        const std::uint64_t x = value_of(obj.nodes[node.inputs.at(0)]);
        const std::uint64_t sh = value_of(obj.nodes[node.inputs.at(1)]);
        const std::uint32_t w = node.width;
        std::uint64_t out = 0;
        if (sh < 64)
          out = (x >> sh) & mask_width(w);
        node.value = out;
        break;
      }
    }
  }
}

}  // namespace gate_cli::sim
