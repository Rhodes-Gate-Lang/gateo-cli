/**
 * File: Simulator.hpp
 * Purpose: `gate sim` command — load .gateo, adapt, run engine.
 */
#pragma once

#include <string>
#include <vector>

#include "Command.hpp"

namespace gate_cli::sim::command {

class Simulator : public Command {
public:
  void run(std::vector<std::string> args) override;
  CommandInfo get_info() override;
};

}  // namespace gate_cli::sim::command
