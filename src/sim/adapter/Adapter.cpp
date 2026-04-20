/**
 * File: Adapter.cpp
 */

#include "sim/adapter/Adapter.hpp"

namespace gate_cli::sim::adapter {

gate_cli::sim::SimObject adapt(const gateo::v2::view::GateObject& obj) {
  gate_cli::sim::SimObject sim;

  // TODO: call validate_basic(obj) and add simulator-specific structural checks
  //       (arity per GateType, width consistency, literal range, etc.).

  // TODO: walk GateObject::components / nodes and classify nodes:
  //       - true top-level inputs/outputs vs grapher-only Input/Output hops inside hierarchy.

  // TODO: build a temporary map from GateObject node index -> "logical producer"
  //       (collapse chains of Input/Output identity nodes so SimObject edges skip them).

  // TODO: maintain an explicit GateObject-index -> SimObject-index table (sparse or dense);
  //       never assume obj.nodes[i] lines up with sim.nodes[i] after folding IO hops.

  // TODO: reserve sim.nodes for the count of nodes that survive lowering (combinational +
  //       any retained boundary nodes such as literals or driven inputs).

  // TODO: for each surviving node, emit SimNode with GateType + width + literal handling;
  //       fill sim.nodes[i].inputs with SimObject indices (not GateObject indices),
  //       using the remap built above.

  // TODO: fill sim.inputs / sim.outputs with port names -> final SimObject node indices.

  // TODO: assert no sim node uses kInvalidNodeIndex unless explicitly documented.

  (void)obj;
  return sim;
}

}  // namespace gate_cli::sim::adapter
