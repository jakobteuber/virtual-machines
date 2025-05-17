#include "lib/CMachine.hpp"
#include "lib/Common.hpp"
#include "lib/Error.hpp"

#include "CMachine.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vm::cma {

static_assert(vm::common::VirtualMachine<CMa>);

void Instr::print(std::span<Instr> instructions) {
  std::println(stderr, "{} instructions", instructions.size());
  for (Instr i : instructions) {
    std::print(stderr, "{}", Instr::toString(i.type));
    bool hasArg = Instr::hasMandatoryArg(i.type) ||
                  (Instr::hasOptionalArg(i.type) && i.arg != 1);
    if (hasArg) {
      std::print(stderr, "\t{}", i.arg);
    }
    std::println(stderr);
  }
}

void CMa::step() {
  Instr instruction = instructions[programCounter];
  programCounter += 1;
  execute(instruction);
}

auto CMa::run() -> int {
  while (programCounter < std::ssize(instructions)) {
    step();
  }
  return memory[0];
}

void CMa::execute(Instr instruction) {
  switch (instruction.type) {
  case Instr::Debug: {
    debug();
  } break;

  case Instr::Loadc: {
    stackPointer += 1;
    memory[stackPointer] = instruction.arg;
  } break;

  case Instr::Add: {
    stackPointer -= 1;
    memory[stackPointer] += memory[stackPointer + 1];
  } break;

  case Instr::Sub: {
    stackPointer -= 1;
    memory[stackPointer] -= memory[stackPointer + 1];
  } break;

  case Instr::Mul: {
    stackPointer -= 1;
    memory[stackPointer] *= memory[stackPointer + 1];
  } break;

  case Instr::Div: {
    stackPointer -= 1;
    memory[stackPointer] /= memory[stackPointer + 1];
  } break;

  case Instr::Mod: {
    stackPointer -= 1;
    memory[stackPointer] %= memory[stackPointer + 1];
  } break;

  case Instr::And: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] && memory[stackPointer + 1];
  } break;

  case Instr::Or: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] || memory[stackPointer + 1];
  } break;

  case Instr::Xor: {
    stackPointer -= 1;
    // Weird non-C semantics: logical exclusive or.
    memory[stackPointer] =
        (memory[stackPointer] != 0) ^ (memory[stackPointer + 1] != 0);
  } break;

  case Instr::Eq: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] == memory[stackPointer + 1];
  } break;

  case Instr::Neq: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] != memory[stackPointer + 1];
  } break;

  case Instr::Gr: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] > memory[stackPointer + 1];
  } break;

  case Instr::Geq: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] >= memory[stackPointer + 1];
  } break;

  case Instr::Le: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] < memory[stackPointer + 1];
  } break;

  case Instr::Leq: {
    stackPointer -= 1;
    memory[stackPointer] = memory[stackPointer] <= memory[stackPointer + 1];
  } break;

  case Instr::Neg: {
    memory[stackPointer] = -memory[stackPointer];
  } break;

  case Instr::Not: {
    memory[stackPointer] = !memory[stackPointer];
  } break;

  case Instr::Load: {
    int dest = memory[stackPointer];
    int count = instruction.arg;
    for (int i = 0; i < count; ++i) {
      memory[stackPointer + i] = memory[dest + i];
    }
    stackPointer += count - 1;
  } break;

  case Instr::Store: {
    int dest = memory[stackPointer];
    int count = instruction.arg;
    for (int i = 0; i < count; ++i) {
      memory[dest + i] = memory[stackPointer - count + i];
    }
    stackPointer -= 1;
  } break;

  case Instr::Loada: {
    stackPointer += 1;
    memory[stackPointer] = memory[instruction.arg];
  } break;

  case Instr::Storea: {
    memory[instruction.arg] = memory[stackPointer];
  } break;

  case Instr::Pop: {
    stackPointer -= instruction.arg;
  } break;

  case Instr::Dup: {
    stackPointer += 1;
    memory[stackPointer] = memory[stackPointer - 1];
  } break;

  case Instr::Jump: {
    programCounter = instruction.arg;
  } break;

  case Instr::Jumpz: {
    if (memory[stackPointer] == 0) {
      programCounter = instruction.arg;
    }
    stackPointer -= 1;
  } break;

  case Instr::Jumpi: {
    programCounter = instruction.arg + memory[stackPointer];
    stackPointer -= 1;
  } break;

  case Instr::Alloc: {
    stackPointer += instruction.arg;
  } break;

  case Instr::New: {
    if (newPointer - memory[stackPointer] <= extremePointer) {
      memory[stackPointer] = 0;
    } else {
      newPointer -= memory[stackPointer];
      memory[stackPointer] = newPointer;
    }
  } break;

  case Instr::Mark: {
    memory[stackPointer + 1] = extremePointer;
    memory[stackPointer + 2] = framePointer;
    stackPointer += 2;
  } break;

  case Instr::Call: {
    int returnAddr = memory[stackPointer];
    memory[stackPointer] = programCounter;
    framePointer = stackPointer;
    programCounter = returnAddr;
  } break;

  case Instr::Slide: {
    int returnValue = memory[stackPointer];
    stackPointer -= instruction.arg;
    memory[stackPointer] = returnValue;
  } break;

  case Instr::Enter: {
    extremePointer = stackPointer + instruction.arg;
    if (extremePointer >= newPointer) {
      debug();
      dbg_fail("Stack overflow");
    }

  } break;

  case Instr::Loadrc: {
    stackPointer += 1;
    memory[stackPointer] = framePointer + instruction.arg;
  } break;

  case Instr::Loadr: {
    int addr = framePointer + instruction.arg;
    stackPointer += 1;
    memory[stackPointer] = memory[addr];
  } break;

  case Instr::Storer: {
    int addr = framePointer + instruction.arg;
    memory[addr] = memory[stackPointer];
  } break;

  case Instr::Return: {
    programCounter = memory[framePointer];
    extremePointer = memory[framePointer - 2];
    if (extremePointer >= newPointer) {
      debug();
      dbg_fail("Stack overflow");
    }
    stackPointer = framePointer - 3;
    framePointer = memory[stackPointer + 2];
  } break;

  case Instr::Halt: {
    programCounter = std::numeric_limits<int>::max();
  } break;

  case Instr::Print: {
    auto x = memory[stackPointer];
    stackPointer -= 1;
    std::println(out, "{}", x);
  } break;

  default:
    dbg_fail("Bad instruction", instruction.type, instruction.arg);
  }
}

void CMa::debug() {
  std::println(out, "CMa state: SP = {}, PC = {}, FP = {}, EP = {}, NP = {}",
               stackPointer, programCounter, framePointer, extremePointer,
               newPointer);
  constexpr int maxStackCount = 10;
  int start = std::max(maxStackCount, stackPointer) - maxStackCount;
  std::print(out, "    stack: ");
  if (start > 0) {
    std::print(out, "...   ");
  }
  for (int i = start; i <= stackPointer; ++i) {
    std::print(out, "{}   ", memory[i]);
  }
  std::println(out, "<- top");
}

auto Instr::hasMandatoryArg(Type t) -> bool {
  return t == Instr::Loadc || t == Instr::Loada || t == Instr::Storea ||
         t == Instr::Jump || t == Instr::Jumpi || t == Instr::Jumpz ||
         t == Instr::Alloc || t == Instr::Enter || t == Instr::Slide ||
         t == Instr::Loadrc || t == Instr::Loadr || t == Instr::Storer;
}

auto Instr::hasOptionalArg(Type t) -> bool {
  return t == Instr::Pop || t == Instr::Load || t == Instr::Store;
}

auto Instr::toString(Instr ::Type enumValue) -> std::string_view {
  constexpr std::array names = {
      "debug", "loadc",  "add",    "sub",   "mul",    "div",    "mod",  "and",
      "or",    "xor",    "eq",     "neq",   "le",     "leq",    "gr",   "geq",
      "not",   "neg",    "load",   "store", "loada",  "storea", "pop",  "jump",
      "jumpz", "jumpi",  "dup",    "alloc", "new",    "mark",   "call", "slide",
      "enter", "return", "loadrc", "loadr", "storer", "halt",   "print"};
  auto index = static_cast<std::size_t>(enumValue);
  dbg_assert(0 <= index && index < names.size(), "Bad enum tag for Instr::Type",
             enumValue);
  return names[index];
}

auto Instr ::fromString(std::string_view name) -> Instr::Type {
  static const std::unordered_map<std::string_view, Instr::Type> names = {
      {"debug", Type::Debug},   {"loadc", Type::Loadc},
      {"add", Type::Add},       {"sub", Type::Sub},
      {"mul", Type::Mul},       {"div", Type::Div},
      {"mod", Type::Mod},       {"and", Type::And},
      {"or", Type::Or},         {"xor", Type::Xor},
      {"eq", Type::Eq},         {"neq", Type::Neq},
      {"le", Type::Le},         {"leq", Type::Leq},
      {"gr", Type::Gr},         {"geq", Type::Geq},
      {"not", Type::Not},       {"neg", Type::Neg},
      {"load", Type::Load},     {"store", Type::Store},
      {"loada", Type::Loada},   {"storea", Type::Storea},
      {"pop", Type::Pop},       {"jump", Type::Jump},
      {"jumpz", Type::Jumpz},   {"jumpi", Type::Jumpi},
      {"dup", Type::Dup},       {"alloc", Type::Alloc},
      {"new", Type::New},       {"mark", Type::Mark},
      {"call", Type::Call},     {"slide", Type::Slide},
      {"enter", Type::Enter},   {"return", Type::Return},
      {"loadrc", Type::Loadrc}, {"loadr", Type::Loadr},
      {"storer", Type::Storer}, {"halt", Type::Halt},
      {"print", Type::Print}};
  std::string canonical = {};
  std::ranges::transform(name, std::back_inserter(canonical),
                         [](unsigned char c) { return std::tolower(c); });
  auto iter = names.find(canonical);
  dbg_assert_neq(iter, names.end(), "Bad enum name for Instr::Type", name,
                 canonical);
  return iter->second;
}

namespace {

class Parser {
  std::string_view text;
  std::size_t position = 0;
  std::int32_t instr_number = 0;
  std::vector<Instr> instructions = {};
  std::unordered_map<std::string_view, std::int32_t> jumpLabels = {};

  enum class Mode : std::uint8_t { GatherLabels, EmitInstructions };
  Mode mode = Mode::GatherLabels;

  auto atEnd() -> bool { return position >= text.size(); }
  auto peek() -> char { return atEnd() ? '\0' : text[position]; }

  auto advance() -> char {
    char c = peek();
    position++;
    return c;
  }

  void consume(char c) {
    if (peek() != c) {
      dbg_fail("Expected different char", peek());
    }
    advance();
  }

  void skipComments() {
    consume('/');
    consume('/');
    while (peek() != '\n' && !atEnd()) {
      advance();
    }
    consume('\n');
  }

  auto isBlank(char c) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  auto isNumeric(char c) -> bool { return ('0' <= c && c <= '9') || c == '-'; }

  auto isIdentStart(char c) -> bool {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
  }

  auto isIdentPart(char c) -> bool { return isIdentStart(c) || isNumeric(c); }

  void skipWhiteSpace() {
    while (isBlank(peek()) && !atEnd()) {
      advance();
    }
  }

  void skip() {
    while (!atEnd()) {
      if (peek() == '/') {
        skipComments();
      } else if (isBlank(peek())) {
        skipWhiteSpace();
      } else {
        break;
      }
    }
  }

  auto consumeColon() -> bool {
    skip();
    bool hasColon = peek() == ':';
    if (hasColon) {
      consume(':');
    }
    skip();
    return hasColon;
  }

  auto readWord() -> std::string_view {
    skip();
    std::size_t start = position;
    while (isIdentPart(peek()) && !atEnd()) {
      advance();
    }
    auto w = text.substr(start, position - start);
    skip();
    return w;
  }

  auto readNumber() -> std::int32_t {
    skip();
    std::size_t start = position;
    if (peek() == '-' || peek() == '+') {
      advance();
    }
    while (isNumeric(peek()) && !atEnd()) {
      advance();
    }
    auto literal = text.substr(start, position - start);
    skip();

    std::int32_t value;
    auto result = std::from_chars(literal.begin(), literal.end(), value);
    dbg_assert_eq(result.ptr, literal.end(), "Could not parse full number",
                  literal);
    return value;
  }

  void registerLabel(std::string_view name) {
    if (mode == Mode::GatherLabels) {
      jumpLabels.insert({name, instr_number});
    }
  }

  void handleInstruction(Instr::Type t, std::int32_t arg = 1) {
    if (mode == Mode::EmitInstructions) {
      instructions.push_back({t, arg});
    }
    instr_number += 1;
  }

  void handleInstruction(Instr::Type t, std::string_view label) {
    if (mode == Mode::EmitInstructions) {
      std::int32_t address = jumpLabels.at(label);
      instructions.push_back({t, address});
    }
    instr_number += 1;
  }

  void parseInstruction(std::string_view word) {
    Instr::Type instructionType = Instr::fromString(word);

    if (Instr::hasMandatoryArg(instructionType)) {
      if (isIdentStart(peek())) {
        handleInstruction(instructionType, readWord());
      } else if (isNumeric(peek())) {
        handleInstruction(instructionType, readNumber());
      } else {
        dbg_fail("bad char", peek());
      }
    } else if (Instr::hasOptionalArg(instructionType)) {
      if (isNumeric(peek())) {
        handleInstruction(instructionType, readNumber());
      } else {
        handleInstruction(instructionType);
      }
    } else {
      handleInstruction(instructionType);
    }
  }

  void consumeWord() {
    auto word = readWord();
    if (consumeColon()) {
      registerLabel(word);
    } else {
      parseInstruction(word);
    }
  }

  void walk(Mode m) {
    mode = m;
    position = 0;
    instr_number = 0;
    instructions.clear();

    for (skip(); !atEnd(); skip()) {
      consumeWord();
    }
  }

public:
  explicit Parser(std::string_view text) : text{text} {}

  auto parse() -> std::vector<Instr> {
    walk(Mode::GatherLabels);
    walk(Mode::EmitInstructions);

    return std::move(instructions);
  }
};

}; // namespace

auto CMa::loadInstructions(std::string_view text) -> std::vector<Instr> {
  return Parser(text).parse();
}

} // namespace vm::cma
