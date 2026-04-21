#include "NumberFormatter.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace gate_cli {

namespace {

std::string_view trim(std::string_view s) {
  // Remove front whitespace
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    s.remove_prefix(1);

  // Remove rear whitespace
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.remove_suffix(1);

  return s;
}


std::uint64_t mask_width(std::uint32_t width) {
  if (width == 0)
    return 0;
  if (width >= 64)
    return ~std::uint64_t{0};

  return (std::uint64_t{1} << width) - 1;
}


bool starts_with_ci(std::string_view s, std::string_view prefix) {
  if (s.size() < prefix.size())
    return false;

  for (std::size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(s[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i])))
      return false;
  }
  return true;
}

}  // namespace


std::uint64_t to_word(std::string_view str_num, std::uint32_t width, std::ostream* warn) {
  const std::string_view s = trim(str_num);

  if (s.empty())
    throw std::invalid_argument("empty number");

  std::uint64_t raw = 0;

  // Binary Check
  if (starts_with_ci(s, "0b")) {
    const std::string_view body = s.substr(2);

    if (body.empty())
      throw std::invalid_argument("binary literal needs digits after 0b");

    if (body.size() > 64)
      throw std::invalid_argument("binary literal exceeds 64 bits");

    for (char c : body) {
      if (c != '0' && c != '1')
        throw std::invalid_argument("binary literal must contain only 0 and 1");

      raw = (raw << 1) | static_cast<std::uint64_t>(c - '0');
    }
  }

  // Hexadecimal check
  else if (starts_with_ci(s, "0x")) {
    const std::string_view body = s.substr(2);

    if (body.empty())
      throw std::invalid_argument("hex literal needs digits after 0x");

    for (char c : body) {
      if (!std::isxdigit(static_cast<unsigned char>(c)))
        throw std::invalid_argument("hex literal contains non-hex digit");
    }

    std::string hex_digits(body);
    raw = std::stoull(hex_digits, nullptr, 16);
  }

  // Decimal check
  else {
    for (char c : s) {
      if (!std::isdigit(static_cast<unsigned char>(c)))
        throw std::invalid_argument("decimal literal must be [0-9]+ only");
    }

    std::string dec_digits(s);
    std::size_t consumed = 0;
    raw = std::stoull(dec_digits, &consumed, 10);

    if (consumed != dec_digits.size())
      throw std::invalid_argument("decimal literal has invalid characters");
  }

  const std::uint64_t mask = mask_width(width);
  const std::uint64_t clipped = raw & mask;

  if (clipped != raw && warn != nullptr)
    *warn << "value truncates to " << width << " bits\n";

  return clipped;
}


std::string word_to_format(std::uint64_t word, NumberFormat format, std::uint32_t width) {
  const std::uint64_t mask = mask_width(width);
  const std::uint64_t w = word & mask;

  switch (format) {
    case NumberFormat::Binary: {
      const int bits = width == 0 ? 1 : (width > 64 ? 64 : static_cast<int>(width));
      std::string out;
      out.reserve(static_cast<std::size_t>(bits) + 2);
      out += "0b";
      for (int i = bits - 1; i >= 0; --i) {
        out += ((w >> i) & 1) ? '1' : '0';
      }
      return out;
    }

    case NumberFormat::Hex: {
      std::ostringstream os;
      os << "0x" << std::hex << std::nouppercase << w;
      return std::move(os).str();
    }

    case NumberFormat::Dec:
      return std::to_string(w);

    default:
      return std::to_string(w);
  }
}

}  // namespace gate_cli
