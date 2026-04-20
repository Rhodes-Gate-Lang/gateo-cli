/**
 * File: Simulator.hpp
 * Purpose: Create the Simulator command class
 */
#pragma once

#include <string>
#include <vector>

#include "Command.hpp"
#include "gateo/v2/view.hpp"

class Simulator : public Command {
private:
  gateo::v2::view::GateObject pathToGateObject(const std::string& path);

public:
  void run(std::vector<std::string> args) override;
  CommandInfo get_info() override;
};
