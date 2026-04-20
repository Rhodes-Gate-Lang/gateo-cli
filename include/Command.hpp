/**
 * File: Command.hpp
 * Purpose: Provides the abstract Command class
 */

#pragma once

#include <string>
#include <vector>

#include "CommandInfo.hpp"

class Command {
public:
  virtual ~Command() = default;

  virtual void run(std::vector<std::string> args) = 0;
  virtual CommandInfo get_info() = 0;
};
