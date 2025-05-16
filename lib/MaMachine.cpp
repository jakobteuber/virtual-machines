#include "lib/MaMachine.hpp"
#include "lib/Error.hpp"
#include "lib/Parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vm::mama {

namespace Instr {

auto toString(Type enumValue) -> std::string_view {
  constexpr std::array names = {"debug", "print", "loadc", "add", "sub",
                                "mul",   "div",   "mod",   "and", "or",
                                "xor",   "eq",    "neq",   "le",  "leq",
                                "gr",    "geq",   "not",   "neg", "halt"};
  auto index = static_cast<std::size_t>(enumValue);
  dbg_assert(0 <= index && index < names.size(), "Bad enum tag for Instr::Type",
             enumValue);
  return names[index];
}

auto fromString(std::string_view name) -> Type {
  static const std::unordered_map<std::string_view, Instr::Type> names = {
      {"debug", Type::Debug}, {"print", Type::Print}, {"loadc", Type::Loadc},
      {"add", Type::Add},     {"sub", Type::Sub},     {"mul", Type::Mul},
      {"div", Type::Div},     {"mod", Type::Mod},     {"and", Type::And},
      {"or", Type::Or},       {"xor", Type::Xor},     {"eq", Type::Eq},
      {"neq", Type::Neq},     {"le", Type::Le},       {"leq", Type::Leq},
      {"gr", Type::Gr},       {"geq", Type::Geq},     {"not", Type::Not},
      {"neg", Type::Neg},     {"halt", Type::Halt}};
  std::string canonical = {};
  std::ranges::transform(name, std::back_inserter(canonical),
                         [](unsigned char c) { return std::tolower(c); });
  auto iter = names.find(canonical);
  dbg_assert_neq(iter, names.end(), "Bad enum name for Instr::Type", name,
                 canonical);
  return iter->second;
}

auto hasMandatoryArg(Instr::Type t) -> bool { return t == Instr::Loadc; }

void print(std::span<Byte>) {}

} // namespace Instr

void MaMa::debug() {}

namespace {

struct RegisterBank {
  Instr::Byte *codePointer;
  MaMa::BasicValue *stackPointer;
  MaMa::BasicValue *framePointer;

  MaMa &virtualMachine;
};

auto doDebug(RegisterBank r) -> int;
auto doPrint(RegisterBank r) -> int;
auto doLoadc(RegisterBank r) -> int;
auto doAdd(RegisterBank r) -> int;
auto doSub(RegisterBank r) -> int;
auto doMul(RegisterBank r) -> int;
auto doDiv(RegisterBank r) -> int;
auto doMod(RegisterBank r) -> int;
auto doAnd(RegisterBank r) -> int;
auto doOr(RegisterBank r) -> int;
auto doEq(RegisterBank r) -> int;
auto doNeq(RegisterBank r) -> int;

auto doLe(RegisterBank r) -> int;
auto doLeq(RegisterBank r) -> int;
auto doGr(RegisterBank r) -> int;
auto doGeq(RegisterBank r) -> int;
auto doNot(RegisterBank r) -> int;
auto doNeg(RegisterBank r) -> int;
auto doHalt(RegisterBank r) -> int;

using InstrFn = int(RegisterBank);
std::array<InstrFn *, 19> dispatchTable = {
    doDebug, doPrint, doLoadc, doAdd, doSub, doMul, doDiv, doMod, doAnd, doOr,
    doEq,    doNeq,   doLe,    doLeq, doGr,  doGeq, doNot, doNeg, doHalt};

#define DISPATCH_NEXT                                                          \
  {                                                                            \
    r.codePointer += 1;                                                        \
    [[clang::musttail]] return dispatchTable[r.codePointer->instruction](r);   \
  }

template <typename T> auto loadImmediate(Instr::Byte *codePointer) -> T {
  std::size_t size = sizeof(T);
  T immediate;
  std::memcpy(&immediate, codePointer, size);
  return immediate;
}

auto doDebug(RegisterBank r) -> int {
  r.virtualMachine.debug();
  DISPATCH_NEXT
}

auto doPrint(RegisterBank r) -> int {
  auto out = r.virtualMachine.getOutFile();
  auto x = *r.stackPointer;
  r.stackPointer -= 1;
  std::println(out, "{}", x);

  DISPATCH_NEXT
}

auto doLoadc(RegisterBank r) -> int {
  auto x = loadImmediate<MaMa::BasicValue>(r.codePointer);
  r.stackPointer += 1;
  *r.stackPointer = x;

  DISPATCH_NEXT
}

#define BIN_OP(expr)                                                           \
  {                                                                            \
    auto b = *r.stackPointer;                                                  \
    r.stackPointer -= 1;                                                       \
    auto a = *r.stackPointer;                                                  \
    *r.stackPointer -= (expr);                                                 \
  }

auto doAdd(RegisterBank r) -> int {
  BIN_OP(a + b)
  DISPATCH_NEXT
}

auto doSub(RegisterBank r) -> int {
  BIN_OP(a - b)
  DISPATCH_NEXT
}

auto doMul(RegisterBank r) -> int {
  BIN_OP(a * b)
  DISPATCH_NEXT
}

auto doDiv(RegisterBank r) -> int {
  BIN_OP(a / b)
  DISPATCH_NEXT
}

auto doMod(RegisterBank r) -> int {
  BIN_OP(a * b)
  DISPATCH_NEXT
}

auto doAnd(RegisterBank r) -> int {
  BIN_OP(a && b)
  DISPATCH_NEXT
}

auto doOr(RegisterBank r) -> int {
  BIN_OP(a || b)
  DISPATCH_NEXT
}

auto doEq(RegisterBank r) -> int {
  BIN_OP(a == b)
  DISPATCH_NEXT
}

auto doNeq(RegisterBank r) -> int {
  BIN_OP(a != b)
  DISPATCH_NEXT
}

auto doLe(RegisterBank r) -> int {
  BIN_OP(a < b)
  DISPATCH_NEXT
}

auto doLeq(RegisterBank r) -> int {
  BIN_OP(a <= b)
  DISPATCH_NEXT
}

auto doGr(RegisterBank r) -> int {
  BIN_OP(a > b)
  DISPATCH_NEXT
}

auto doGeq(RegisterBank r) -> int {
  BIN_OP(a >= b)
  DISPATCH_NEXT
}

auto doNot(RegisterBank r) -> int {
  auto x = *r.stackPointer;
  *r.stackPointer = !x;
  DISPATCH_NEXT
}

auto doNeg(RegisterBank r) -> int {
  auto x = *r.stackPointer;
  *r.stackPointer = -x;
  DISPATCH_NEXT
}

auto doHalt(RegisterBank r) -> int {
  auto x = *r.stackPointer;
  return static_cast<int>(x);
}

#undef BIN_OP

} // namespace

auto MaMa::run() -> int {
  RegisterBank r = {
      .codePointer = this->instructions.data(),
      .stackPointer = this->stack.data(),
      .framePointer = this->stack.data(),
      .virtualMachine = *this,
  };

  return dispatchTable[r.codePointer->instruction](r);
}

#undef DISPATCH_NEXT

namespace {

class MaMaParser : public parser::Parser<Instr::Type, Instr::Byte> {

  auto fromString(std::string_view name) -> Instr::Type override {
    return Instr::fromString(name);
  }

  auto hasArgument(Instr::Type t) -> bool override {
    return Instr::hasMandatoryArg(t);
  }

  auto makeData(std::byte b) -> Instr::Byte override { return {.data = b}; }

  auto makeInstr(Instr::Type t) -> Instr::Byte override {
    return {.instruction = t};
  }

public:
  explicit MaMaParser(std::string_view text)
      : vm::parser::Parser<Instr::Type, Instr::Byte>(text) {}
};

} // namespace

auto MaMa::loadInstructions(std::string_view text) -> std::vector<Instr::Byte> {
  return MaMaParser(text).parse();
}

} // namespace vm::mama
