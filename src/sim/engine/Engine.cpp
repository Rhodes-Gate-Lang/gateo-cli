/**
 * File: Engine.cpp
 */

#include "sim/engine/Engine.hpp"

namespace gate_cli::sim::engine {

void eval(gate_cli::sim::SimObject& sim) {
  // TODO: require sim produced by adapter::adapt (well-formed graph, topo order).

  // TODO: apply stimulus: for each entry in sim.inputs, write the bound node's `out`
  //       according to the agreed width / word layout.

  // TODO: forward pass in topological order: for each SimNode, read fan-in `out` slices,
  //       dispatch on GateType, write this node's `out`.

  // TODO: for sim.outputs, either leave consumers to read by index or materialize a view.

  (void)sim;
}

}  // namespace gate_cli::sim::engine
