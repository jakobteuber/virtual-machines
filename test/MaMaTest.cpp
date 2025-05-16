#include <gtest/gtest.h>
#include <string_view>

#include "lib/Error.hpp"
#include "lib/MaMachine.hpp"

namespace {

auto run(std::string_view text) -> std::string {
  using vm::mama::MaMa;

  auto instructions = MaMa::loadInstructions(text);

  FILE *f = std::tmpfile();
  dbg_assert_neq(f, nullptr, "could not open file");
  auto vm = MaMa(instructions, f);
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

TEST(MaMa, empty) { ASSERT_EQ(run("halt"), ""); }

TEST(MaMa, SimpleAdd) {
  ASSERT_EQ(run("loadc 10 loadc 10 add print halt"), "20\n");
}

TEST(MaMa, SimpleSub) {
  ASSERT_EQ(run("loadc 60 loadc 50 sub print halt"), "10\n");
}

TEST(MaMa, Loop) {
  std::string_view prog = R"(
      loadc 10
   L: loadc 1
      sub
      dup
      print 
      dup
      jumpz E 
      jump L 
   E: halt
  )";
  ASSERT_EQ(run(prog), "9\n8\n7\n6\n5\n4\n3\n2\n1\n0\n");
}
