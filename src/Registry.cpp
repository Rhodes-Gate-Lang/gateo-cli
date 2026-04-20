/**
 * File: Registry.cpp
 * Purpose: Implementation of the command registry
 */

#include "Registry.hpp"

#include "sim/Simulator.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

Registry::Registry() {
  registry_.emplace("sim", std::make_unique<gate_cli::sim::Simulator>());
}

void Registry::print_help() {
  std::vector<CommandInfo> outputs;
  outputs.reserve(registry_.size());

  for (const auto& [_, command] : registry_) {
    outputs.push_back(command->get_info());
  }

  std::sort(outputs.begin(), outputs.end());

  for (const auto& info : outputs) {
    std::cout << info << "\n\n";
  }
}

void Registry::run_command(std::string name, std::vector<std::string> args) {
  if (name == "help") {
    print_help();
    return;
  }

  if (!registry_.count(name)) {
    std::cout << "Command \"" << name << "\" not found.\n";
    print_help();
    return;
  }

  const auto& cmd = registry_.at(name);

  cmd->run(std::move(args));
}
