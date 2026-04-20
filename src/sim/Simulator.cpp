/**
 * File: Simulator.cpp
 * Purpose: Simulator command — load .gateo via gateo-cpp
 */

#include "sim/Simulator.hpp"

#include "gateo/io.hpp"

#include <filesystem>
#include <iostream>

using gateo::v2::view::GateObject;

GateObject Simulator::pathToGateObject(const std::string& path) {
  return gateo::read_file(std::filesystem::path(path));
}

void Simulator::run(std::vector<std::string> args) {
  if (args.empty()) {
    std::cerr << "usage: gate sim <file.gateo>\n";
    return;
  }
  try {
    const GateObject obj = pathToGateObject(args[0]);
    std::cout << "sim: loaded \"" << args[0] << "\" — " << obj.components.size()
              << " component(s), " << obj.nodes.size() << " node(s)\n";
  } catch (const std::exception& e) {
    std::cerr << "sim: " << e.what() << "\n";
  }
}

CommandInfo Simulator::get_info() {
  return CommandInfo{"sim",
                     "Load a .gateo design file (gateo.v2) for inspection or simulation",
                     "gate sim <file.gateo>"};
}
