#include <cstddef>
#include <gtest/gtest.h>

#include "lib/CMachine.hpp"
#include "lib/Error.hpp"

#include <cstdio>
#include <string>
#include <string_view>

using namespace vm::cma;

namespace {

auto runWithExitCode(std::string_view text) -> int {
  auto instructions = CMa::loadInstructions(text);
  auto vm = CMa(instructions);
  return vm.run();
}

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

TEST(CMa, empty) { ASSERT_EQ(run("halt"), ""); }

TEST(CMa, count) {
  std::string_view expected = "9\n8\n7\n6\n5\n4\n3\n2\n1\n0\n";
  std::string_view program = R"(
        loadc 10
  loop: loadc 1
        sub
        dup
        print
        dup 
        jumpz end
        jump loop 
  end:  halt
  )";
  ASSERT_EQ(run(program), expected);
}

TEST(CMa, switch) {
  std::string_view program = R"(
          loadc 2        
          dup
          loadc 0 
          geq 
          jumpz A 
          dup 
          loadc 3 
          le 
          jumpz A 
          jumpi B 
      A:  pop
          loadc 3 
          jumpi B 

      C0: loadc 0 print jump D 
      C1: loadc 1 print jump D 
      C2: loadc 2 print jump D 
      C3: loadc 3 print jump D 

      B: jump C0 jump C1 jump C2 jump C3 

      D: halt 
  )";
  ASSERT_EQ(run(program), "2\n");
}

TEST(CMa, while) {
  std::string_view program = R"(
          loadc 1000 
          loadc 0 
      L:  loada 0 
          loada 1 
          neq 
          jumpz E 
          loada 1 
          loadc 1 
          add 
          storea 1
          pop 
          jump L 
      E:  loada 1 
          print 
          halt
  )";
  ASSERT_EQ(run(program), "1000\n");
}

TEST(CMa, if) {
  std::string_view program = R"(
          loadc 1 
          loadc 10 
          gr 
          jumpz E 
          loadc 0 
          print 
      E:  loadc 1 
          print 
          halt 
  )";
  ASSERT_EQ(run(program), "1\n");
}

TEST(CMa, new) {
  std::string_view program = R"(
         loadc 100 
         new 
         dup 
         loadc 11 
         loada 0 
         store 
         pop 
         load 
         print 
         halt 
  )";
  ASSERT_EQ(run(program), "11\n");
}

TEST(CMa, factorialFull) {
  std::string_view program = R"(
          enter 4 
          alloc 1 
          mark 
          loadc _main 
          call 
          slide 0 
          halt 
  _fac:   enter 5 
          loadr -3 
          loadc 0 
          leq 
          jumpz A 
          loadc 1 
          storer -3 
          return 
          jump B 
      A:  loadr -3 
          loadr -3 
          loadc 1 
          sub 
          mark 
          loadc _fac 
          call 
          slide 0 
          mul 
          storer -3 
          return 
      B:  return
  _main:  enter 4 
          loadc 5    // argument of fac 
          mark 
          loadc _fac 
          call 
          slide 0 
          storer -3 
          return 
  )";
  ASSERT_EQ(runWithExitCode(program), 120);
}
