#include "lib/Common.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>

namespace vm::common {

auto readFile(std::string_view name) -> std::string {
  const std::filesystem::path path = name;
  if (!std::filesystem::exists(path)) {
    std::println(stderr, "Cannot open file: {}", name);
  }
  std::ifstream in {path};
  auto begin = std::istreambuf_iterator<char>(in);
  auto end = std::istreambuf_iterator<char>();
  return std::string(begin, end);
}

} // namespace vm::common
