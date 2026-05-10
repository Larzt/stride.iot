#include "sink.hpp"

void sink_file(const std::string &path, const std::string &text)
{

  FILE *f = fopen(path.c_str(), "a");

  if (f == NULL)
  {
    StrideLogger::Error(StrideSubsystem::Interpreter, "Could not open the file: %s", path.c_str());
    return;
  }

  fprintf(f, "%s", text.c_str());
  fclose(f);
}
