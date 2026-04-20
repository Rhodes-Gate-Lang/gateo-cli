#include "Registry.hpp"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
  std::vector<std::string> positional;
  positional.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
  for (int i = 1; i < argc; ++i) {
    positional.emplace_back(argv[i]);
  }

  if (positional.empty()) {
    std::cerr << "usage: gate <command> [args...]\n";
    std::cerr << "Try: gate help\n";
    return 1;
  }

  std::string cmd = std::move(positional.front());
  positional.erase(positional.begin());

  Registry registry;
  registry.run_command(std::move(cmd), std::move(positional));
  return 0;
}
