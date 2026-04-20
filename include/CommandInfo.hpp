/**
 * File: CommandInfo.hpp
 * Purpose: Defines the CommandInfo struct to pull info about a command from its registry entry
 */
#pragma once

#include <ostream>
#include <string>

struct CommandInfo {
  std::string name;
  std::string desc;
  std::string usage;

  bool operator<(const CommandInfo& other) const {
    return name < other.name;
  }  // Sortable
};

inline std::ostream& operator<<(std::ostream& os, const CommandInfo& info) {
  os << info.name << "\n  " << info.desc << "\n  Usage: " << info.usage;
  return os;
}
