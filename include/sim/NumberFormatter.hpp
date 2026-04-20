/**
 * File: NumberFormatter.hpp
 * Purpose: Convert between CLI number strings and uint64 words for simulation nodes.
 */

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace gate_cli::sim {

enum class NumberFormat { Binary, Dec, Hex };

/// Parses `str_num` into a value masked to `width` bits (width clamped to 64).
/// If non-null, `warn` receives a one-line message when significant bits are truncated.
std::uint64_t to_word(std::string_view str_num, std::uint32_t width, std::ostream* warn = nullptr);

/// Formats a word for display; binary is zero-padded to `width` bits (width 0 → minimal).
std::string word_to_format(std::uint64_t word, NumberFormat format, std::uint32_t width = 64);

}  // namespace gate_cli::sim
