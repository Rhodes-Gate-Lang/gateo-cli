#include "codegen/CodegenConfig.hpp"

#include "gateo/io.hpp"

#include <filesystem>

namespace gate_cli::codegen {

void CodegenConfig::load_object() {
  gate_object = gateo::read_file(std::filesystem::path(gateo_path));
}

}  // namespace gate_cli::codegen
