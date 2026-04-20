/**
 * File: Simulator.cpp
 */

#include "sim/command/Simulator.hpp"

#include "sim/adapter/Adapter.hpp"
#include "sim/engine/Engine.hpp"

#include "gateo/io.hpp"

#include <filesystem>
#include <iostream>

namespace gate_cli::sim::command {

namespace {

gateo::v2::view::GateObject path_to_gate_object(const std::string& path) {
  return gateo::read_file(std::filesystem::path(path));
}

}  // namespace

void Simulator::run(std::vector<std::string> args) {
  if (args.empty()) {
    std::cerr << "usage: gate sim <file.gateo>\n";
    return;
  }
  try {
    const gateo::v2::view::GateObject obj = path_to_gate_object(args[0]);
    std::cout << "sim: loaded \"" << args[0] << "\" — " << obj.components.size()
              << " component(s), " << obj.nodes.size() << " node(s)\n";

    gate_cli::sim::SimObject sim = gate_cli::sim::adapter::adapt(obj);
    std::cout << "sim: lowered graph — " << sim.nodes.size() << " sim node(s)\n";

    gate_cli::sim::engine::eval(sim);
  } catch (const std::exception& e) {
    std::cerr << "sim: " << e.what() << "\n";
  }
}

CommandInfo Simulator::get_info() {
  return CommandInfo{"sim",
                     "Load a .gateo design file (gateo.v2) for inspection or simulation",
                     "gate sim <file.gateo>"};
}

}  // namespace gate_cli::sim::command
