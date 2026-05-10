#include "hexer.hpp"

int parse_hex_number(const std::string &hex)
{
  if (hex.find("0x") == 0)
  {
    return std::stoi(hex, nullptr, 16);
  }
  return std::stoi(hex);
}
