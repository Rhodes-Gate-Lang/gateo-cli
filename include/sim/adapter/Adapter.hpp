/**
 * File: Adapter.hpp
 * Purpose: GateObject -> SimObject lowering (fold IO intermediaries, remap indices).
 */
#pragma once

#include "gateo/v2/view.hpp"
#include "sim/SimObject.hpp"

namespace gate_cli::sim::adapter {

/// Build a `SimObject` from a loaded design. GateObject node order is preserved
/// only insofar as the lowered graph requires; indices are not 1:1 with GateObject.
gate_cli::sim::SimObject adapt(const gateo::v2::view::GateObject& obj);

}  // namespace gate_cli::sim::adapter
