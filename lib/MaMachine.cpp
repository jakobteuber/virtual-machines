#include "lib/MaMachine.hpp"
#include "lib/Error.hpp"
#include "lib/Parser.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vm::mama {

namespace {

template <typename T, std::size_t N>
constexpr auto
checkedArray(std::initializer_list<std::pair<std::size_t, T>> init)
    -> std::array<T, N> {
  dbg_assert_eq(N, init.size(), "wrong number of elements", N, init.size());

  std::array<bool, N> isSet = {};
  std::array<T, N> arr = {};

  std::ranges::fill(isSet, false);

  for (auto [index, value] : init) {
    dbg_assert(!isSet.at(index), "double index", index);
    isSet[index] = true;
    arr[index] = value;
  }

  for (std::size_t i = 0; i < N; ++i) {
    dbg_assert(isSet.at(i), "unset element");
  }

  return arr;
}

} // namespace

namespace Instr {

auto toString(Type enumValue) -> std::string_view {
  constexpr std::array names = {
      "debug", "print",    "loadc",   "add",     "sub",      "mul",  "div",
      "mod",   "and",      "or",      "xor",     "eq",       "neq",  "le",
      "leq",   "gr",       "geq",     "not",     "neg",      "halt", "jump",
      "jumpz", "getbasic", "mkbasic", "pushloc", "pushglob", "slide"};
  auto index = static_cast<std::size_t>(enumValue);
  dbg_assert(0 <= index && index < names.size(), "Bad enum tag for Instr::Type",
             enumValue);
  return names[index];
}

auto fromString(std::string_view name) -> Type {
  static const std::unordered_map<std::string_view, Instr::Type> names = {
      {"debug", Type::Debug},       {"print", Type::Print},
      {"loadc", Type::Loadc},       {"add", Type::Add},
      {"sub", Type::Sub},           {"mul", Type::Mul},
      {"div", Type::Div},           {"mod", Type::Mod},
      {"and", Type::And},           {"or", Type::Or},
      {"xor", Type::Xor},           {"eq", Type::Eq},
      {"neq", Type::Neq},           {"le", Type::Le},
      {"leq", Type::Leq},           {"gr", Type::Gr},
      {"geq", Type::Geq},           {"not", Type::Not},
      {"neg", Type::Neg},           {"halt", Type::Halt},
      {"jump", Type::Jump},         {"jumpz", Type::Jumpz},
      {"getbasic", Type::Getbasic}, {"mkbasic", Type::Mkbasic},
      {"pushloc", Type::Pushloc},   {"pushglob", Type::Pushglob},
      {"slide", Type::Slide}};
  std::string canonical = {};
  std::ranges::transform(name, std::back_inserter(canonical),
                         [](unsigned char c) { return std::tolower(c); });
  auto iter = names.find(canonical);
  dbg_assert_neq(iter, names.end(), "Bad enum name for Instr::Type", name,
                 canonical);
  return iter->second;
}

auto hasMandatoryArg(Instr::Type t) -> bool {
  return t == Loadc || t == Jump || t == Jumpz || t == Pushloc ||
         t == Pushglob || t == Slide;
}

void print(std::span<Byte> code) {
  std::println(stderr, "Instructions ({} bytes):", code.size());
  for (std::size_t i = 0; i < code.size();) {
    auto instr = code[i++].instruction;
    std::print(stderr, "   {} ", toString(instr));
    if (hasMandatoryArg(instr)) {
      std::int64_t value = 0;
      dbg_assert(i + sizeof(value) < code.size(),
                 "not enough space for immediate in code store", i,
                 code.size());
      std::memcpy(&value, &code[i], sizeof(value));
      i += sizeof(value);
      std::print(stderr, "{} ", value);
    }
    std::println(stderr);
  }
}

} // namespace Instr

void MaMa::debug() {}

auto MaMa::createNew(HeapValue &&v) -> HeapValue * {
  heap.push_front(v);
  return &heap.front();
}

namespace {

struct RegisterBank {
  Instr::Byte *codePointer;
  MaMa::BasicValue *stackPointer;
  MaMa::BasicValue *framePointer;
  MaMa::BasicValue *globalPointer;

  MaMa &virtualMachine;
};

static_assert(sizeof(RegisterBank) <= 8 * sizeof(void *),
              "With gnu::regcall we have at least 8 registers to our disposal");

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
auto doXor(RegisterBank r) -> int;
auto doEq(RegisterBank r) -> int;
auto doNeq(RegisterBank r) -> int;

auto doLe(RegisterBank r) -> int;
auto doLeq(RegisterBank r) -> int;
auto doGr(RegisterBank r) -> int;
auto doGeq(RegisterBank r) -> int;
auto doNot(RegisterBank r) -> int;
auto doNeg(RegisterBank r) -> int;
auto doHalt(RegisterBank r) -> int;

auto doJump(RegisterBank r) -> int;
auto doJumpz(RegisterBank r) -> int;
auto doGetbasic(RegisterBank r) -> int;
auto doMkbasic(RegisterBank r) -> int;

auto doPushloc(RegisterBank r) -> int;
auto doPushglob(RegisterBank r) -> int;
auto doSlide(RegisterBank r) -> int;

using InstrFn = int(RegisterBank);
std::array dispatchTable = checkedArray<InstrFn *, 27>(
    {{Instr::Debug, doDebug},       {Instr::Print, doPrint},
     {Instr::Loadc, doLoadc},       {Instr::Add, doAdd},
     {Instr::Sub, doSub},           {Instr::Mul, doMul},
     {Instr::Div, doDiv},           {Instr::Mod, doMod},
     {Instr::And, doAnd},           {Instr::Or, doOr},
     {Instr::Xor, doXor},           {Instr::Eq, doEq},
     {Instr::Neq, doNeq},           {Instr::Le, doLe},
     {Instr::Leq, doLeq},           {Instr::Gr, doGr},
     {Instr::Geq, doGeq},           {Instr::Not, doNot},
     {Instr::Neg, doNeg},           {Instr::Halt, doHalt},
     {Instr::Jump, doJump},         {Instr::Jumpz, doJumpz},
     {Instr::Getbasic, doGetbasic}, {Instr::Mkbasic, doMkbasic},
     {Instr::Pushglob, doPushglob}, {Instr::Pushloc, doPushloc},
     {Instr::Slide, doSlide}});

#define DISPATCH_NEXT                                                          \
  {                                                                            \
    auto next = dispatchTable[r.codePointer->instruction];                     \
    r.codePointer += 1;                                                        \
    [[clang::musttail]] return next(r);                                        \
  }

template <typename T>
[[gnu::always_inline]] auto loadImmediate(Instr::Byte *&codePointer) -> T {
  T immediate;
  std::memcpy(&immediate, codePointer, sizeof(T));

  codePointer += sizeof(T);
  return immediate;
}

auto doDebug(RegisterBank r) -> int {
  r.virtualMachine.debug();
  DISPATCH_NEXT
}

auto doPrint(RegisterBank r) -> int {
  auto out = r.virtualMachine.getOutFile();
  auto x = r.stackPointer->value;
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
    std::int64_t b = r.stackPointer->value;                                    \
    r.stackPointer -= 1;                                                       \
    std::int64_t a = r.stackPointer->value;                                    \
    std::int64_t result = (expr);                                              \
    r.stackPointer->value = result;                                            \
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

auto doXor(RegisterBank r) -> int {
  BIN_OP((a != 0) != (b != 0))
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

#undef BIN_OP

auto doNot(RegisterBank r) -> int {
  auto x = r.stackPointer->value;
  r.stackPointer->value = !x;
  DISPATCH_NEXT
}

auto doNeg(RegisterBank r) -> int {
  auto x = r.stackPointer->value;
  r.stackPointer->value = -x;
  DISPATCH_NEXT
}

auto doHalt(RegisterBank r) -> int {
  auto exitCode = static_cast<int>(r.stackPointer->value);
  return exitCode;
}

auto doJump(RegisterBank r) -> int {
  auto pc = loadImmediate<std::size_t>(r.codePointer);
  r.codePointer = r.virtualMachine.getCodeStart() + pc;
  DISPATCH_NEXT;
}

auto doJumpz(RegisterBank r) -> int {
  auto pc = loadImmediate<std::size_t>(r.codePointer);
  auto x = r.stackPointer->value;
  r.stackPointer -= 1;
  if (x == 0) {
    r.codePointer = r.virtualMachine.getCodeStart() + pc;
  }
  DISPATCH_NEXT
}

auto doGetbasic(RegisterBank r) -> int {
  auto x = *r.stackPointer;
  r.stackPointer -= 1;
  auto *heapPointer = std::bit_cast<MaMa::HeapValue *>(x);
  auto &object = *heapPointer;

  if (!std::holds_alternative<MaMa::BasicValue>(object)) {
    dbg_fail("Bad Object Tag", object.index());
  }

  auto basic = std::get<MaMa::BasicValue>(object);
  r.stackPointer += 1;
  *r.stackPointer = basic;

  DISPATCH_NEXT
}

auto doMkbasic(RegisterBank r) -> int {
  auto x = *r.stackPointer;
  r.stackPointer -= 1;
  r.virtualMachine.createNew(x);

  DISPATCH_NEXT
}

auto doPushloc(RegisterBank r) -> int {
  auto n = loadImmediate<std::size_t>(r.codePointer);
  auto ptr = *(r.stackPointer - n);
  r.stackPointer += 1;
  *r.stackPointer = ptr;

  DISPATCH_NEXT
}

auto doPushglob(RegisterBank r) -> int {
  auto n = loadImmediate<std::size_t>(r.codePointer);
  r.stackPointer += 1;
  *r.stackPointer = r.globalPointer[n];

  DISPATCH_NEXT
}

auto doSlide(RegisterBank r) -> int {
  auto top = *r.stackPointer;
  r.stackPointer -= loadImmediate<std::size_t>(r.codePointer);
  *r.stackPointer = top;

  DISPATCH_NEXT
}

auto startDispatching(RegisterBank r) -> int { DISPATCH_NEXT }

#undef DISPATCH_NEXT

} // namespace

auto MaMa::run() -> int {
  RegisterBank r = {
      .codePointer = this->instructions.data(),
      .stackPointer = this->stack.data(),
      .framePointer = this->stack.data(),
      .globalPointer = this->gloabls.data(),
      .virtualMachine = *this,
  };

  return startDispatching(r);
}

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
  std::vector code = MaMaParser(text).parse();
  Instr::print(code);
  return code;
}

} // namespace vm::mama
