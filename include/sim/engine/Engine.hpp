/**
 * File: Engine.hpp
 * Purpose: Evaluate a adapted SimObject (combinational forward pass, etc.).
 */
#pragma once

#include "sim/SimObject.hpp"

namespace gate_cli::sim::engine {

/// Single evaluation pass over `sim` (mutates node `out` state).
void eval(gate_cli::sim::SimObject& sim);

}  // namespace gate_cli::sim::engine
