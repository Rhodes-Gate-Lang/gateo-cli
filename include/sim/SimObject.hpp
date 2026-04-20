/**
 * File: SimObject.hpp
 * Purpose: Runtime simulation graph; indices reference SimObject::nodes.
 */
#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "gateo/v2/view.hpp"

namespace gate_cli::sim {

inline constexpr std::uint32_t kInvalidNodeIndex = std::numeric_limits<std::uint32_t>::max();

struct SimNode {
  gateo::v2::view::GateType type{};
  /// Fan-in: each value is an index into `SimObject::nodes` (not a GateObject index).
  std::vector<std::uint32_t> inputs;
  std::uint32_t width{};
  /// Output state; word packing for `width` is defined by the engine.
  std::vector<std::uint64_t> out;
};

struct SimObject {
  /// Named top-level input port -> node index whose `out` the engine drives.
  std::vector<std::pair<std::string, std::uint32_t>> inputs;
  std::vector<SimNode> nodes;
  /// Named top-level output port -> node index to observe (after eval).
  std::vector<std::pair<std::string, std::uint32_t>> outputs;
};

}  // namespace gate_cli::sim
