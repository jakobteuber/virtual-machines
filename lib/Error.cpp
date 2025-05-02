#include "lib/Error.hpp"

#include <cstddef>
#include <cstdlib>
#include <print>
#include <string_view>

#include <backward.hpp>

namespace {

const backward::SignalHandling s = {};

void printStackTrace() {
  using namespace backward;
  StackTrace s;
  constexpr std::size_t max_depth = 30;
  s.load_here(max_depth);
  Printer().print(s);
}

} // namespace

namespace jakobteuber::util::error {

[[noreturn]] void assertError(std::string_view msg, std::string_view expr,
                              std::string_view info, std::string_view file,
                              std::size_t line) {
  std::println("\033[1;31m{}:{}:\033[0m", file, line);
  std::println("\033[31m{}", msg);
  std::println("\t{}", expr);
  std::println("{}", info);
  std::println("\033[0m");

  printStackTrace();
  std::exit(EXIT_FAILURE);
}

} // namespace jakobteuber::util::error
