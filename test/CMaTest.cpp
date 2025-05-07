#include <gtest/gtest.h>

#include "lib/CMachine.hpp"
#include "test/stdcapture.h"

#include <cstddef>
#include <string>
#include <string_view>

using namespace vm::cma;

namespace {

auto run(std::string_view instructions) -> std::string {
  using capture::CaptureStdout;
  std::string actualOutput;
  CaptureStdout capture([&](const char* buf, std::size_t szbuf) {
    actualOutput += std::string(buf, szbuf);
  });
  CMa vm = CMa::loadInstructions(instructions);
  vm.run();
  return actualOutput;
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
