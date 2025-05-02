#include "lib/Common.hpp"

#include "Common.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <print>

std::string vm::common::readFile(std::string_view name) {
  std::filesystem::path path = name;
  if (!std::filesystem::exists(path)) {
    std::println(stderr, "Cannot open file: {}", name);
  }
  std::ifstream in {path};
  auto begin = std::istreambuf_iterator<char>(in);
  auto end = std::istreambuf_iterator<char>();
  return std::string(begin, end);
}
