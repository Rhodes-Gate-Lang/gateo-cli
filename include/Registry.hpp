/**
 * File: Registry.hpp
 * Purpose: Header file for the command registry
 */
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Command.hpp"

class Registry {
private:
  std::unordered_map<std::string, std::unique_ptr<Command>> registry_;

  void print_help();

public:
  explicit Registry();

  void run_command(std::string name, std::vector<std::string> args);
};
