#include "lib/CMachine.hpp"
#include "lib/Common.hpp"

#include <filesystem>
#include <print>
#include <string_view>

using namespace vm::cma;
using namespace vm::common;

void wrongUsage(std::string_view program_name) {
  std::println(
      stderr, "{} <FILE> – Run the file’s VM-instructions", program_name
  );
  std::exit(EXIT_FAILURE);
}

int main(int argc, char const* argv[]) {
  if (argc != 2) { wrongUsage(argv[0]); }
  std::string_view filename = argv[1];
  std::string text = readFile(filename);

  CMa machine = CMa::loadInstructions(text);
  (void)machine;
}
