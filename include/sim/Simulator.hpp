#pragma once

#include <string>
#include <vector>

#include "Command.hpp"

namespace gate_cli::sim {

class Simulator : public Command {
public:
  void run(std::vector<std::string> args) override;
  CommandInfo get_info() override;
};

}
