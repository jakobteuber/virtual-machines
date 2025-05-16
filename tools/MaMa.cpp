#include "lib/Common.hpp"
#include "lib/MaMachine.hpp"

#include <cstdlib>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace {

void wrongUsage(std::string_view program_name) {
  std::println(stderr, "{} <FILE> – Run the file’s VM-instructions",
               program_name);
  std::exit(EXIT_FAILURE);
}

auto run(std::string_view filename) -> int {
  using vm::common::readFile;
  using vm::mama::MaMa;

  const std::string text = readFile(filename);

  std::vector instructions = MaMa::loadInstructions(text);
  auto machine = MaMa(instructions, stdout);
  int exit = machine.run();
  return exit;
}

} // namespace

auto main(int argc, char const *argv[]) -> int {
  try {
    if (argc != 2) {
      wrongUsage(argv[0]);
    }
    std::string_view filename = argv[1];
    run(filename);
  } catch (...) {
    return EXIT_FAILURE;
  }
}
