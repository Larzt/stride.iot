#include "parse.hpp"

bool is_executable_program(const std::string &file)
{
  if (file.length() < 4)
    return false;

  std::string ext = file.substr(file.length() - 4);

  return (ext == ".str" || ext == ".STR");
}
