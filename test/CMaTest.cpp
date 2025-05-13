#include <cstddef>
#include <gtest/gtest.h>

#include "lib/CMachine.hpp"
#include "lib/Error.hpp"

#include <cstdio>
#include <string>
#include <string_view>

using namespace vm::cma;

namespace {

auto run(std::string_view text) -> std::string {
  auto instructions = CMa::loadInstructions(text);

  FILE *f = std::tmpfile();
  dbg_assert_neq(f, nullptr, "could not open file");
  auto vm = CMa(instructions, f);
  vm.run();

  std::fseek(f, 0, SEEK_END);
  std::size_t size = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);

  std::string output;
  output.resize(size + 1);
  std::fread(output.data(), 1, size, f);
  output.back() = '\0';
  output.resize(size);

  std::fclose(f);

  return output;
}

} // namespace

TEST(CMa, empty) {
  std::string expected = R"()";
  ASSERT_EQ(run(""), expected);
}

TEST(CMa, start) {
  std::string expected =
      "CMa state: SP = 0, PC = 1, FP = 0, EP = 0, NP = 1048575\n"
      "    stack: 0   <- top\n";
  ASSERT_EQ(run("debug"), expected);
}

TEST(CMa, countAndPush) {
  std::string expected =
      "CMa state: SP = 13, PC = 8, FP = 0, EP = 0, NP = 1048575\n"
      "    stack: ...   10   9   8   7   6   5   4   3   2   1   0   <- top\n";
  std::string program = R"(
        loadc 12
loop:   dup
        loadc 1
        sub
        dup
        jumpz end
        jump loop 
end:    debug
  )";
  ASSERT_EQ(run(program), expected);
}

TEST(CMa, alloc) {
  std::string expected =
      "CMa state: SP = 10, PC = 2, FP = 0, EP = 0, NP = 1048575\n"
      "    stack: 0   0   0   0   0   0   0   0   0   0   0   <- top\n";
  std::string program = "alloc 10 debug";
  ASSERT_EQ(run(program), expected);
}
