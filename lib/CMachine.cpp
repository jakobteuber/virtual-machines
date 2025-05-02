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
#include <iterator>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vm::cma {

static_assert(vm::common::VirtualMachine<CMa>);

namespace {

constexpr std::array<std::string_view, 27> names = {
    "LOADC", "ADD",  "SUB",   "MUL",   "DIV",   "MOD",   "AND",
    "OR",    "XOR",  "EQ",    "NEQ",   "LE",    "LEQ",   "GR",
    "GEQ",   "NOT",  "NEG",   "LOAD",  "STORE", "LOADA", "STOREA",
    "POP",   "JUMP", "JUMPZ", "JUMPI", "DUP",   "ALLOC"};

static const std::unordered_map<std::string_view, Instr::Type> namesToEnum = {
    {"LOADC",  Instr::Loadc },
    {"ADD",    Instr::Add   },
    {"SUB",    Instr::Sub   },
    {"MUL",    Instr::Mul   },
    {"DIV",    Instr::Div   },
    {"MOD",    Instr::Mod   },
    {"AND",    Instr::And   },
    {"OR",     Instr::Or    },
    {"XOR",    Instr::Xor   },
    {"EQ",     Instr::Eq    },
    {"NEQ",    Instr::Neq   },
    {"LE",     Instr::Le    },
    {"LEQ",    Instr::Leq   },
    {"GR",     Instr::Gr    },
    {"GEQ",    Instr::Geq   },
    {"NOT",    Instr::Not   },
    {"NEG",    Instr::Neg   },
    {"LOAD",   Instr::Load  },
    {"STORE",  Instr::Store },
    {"LOADA",  Instr::Loada },
    {"STOREA", Instr::Storea},
    {"POP",    Instr::Pop   },
    {"JUMP",   Instr::Jump  },
    {"JUMPZ",  Instr::Jumpz },
    {"JUMPI",  Instr::Jumpi },
    {"DUP",    Instr::Dup   },
    {"ALLOC",  Instr::Alloc }
};

}; // namespace

auto Instr::toString(Type t) -> std::string_view {
  auto index = static_cast<unsigned>(t);
  dbg_assert(0 <= index && index < names.size(), "Bad enum tag", t);
  return names[index];
}

auto Instr::fromString(std::string_view name) -> Instr::Type {
  std::string canonical = {};
  std::ranges::transform(name, std::back_inserter(canonical),
                         [](unsigned char c) { return std::toupper(c); });
  auto iter = namesToEnum.find(canonical);

  dbg_assert_neq(iter, namesToEnum.end(), "Bad enum constant", name, canonical);
  return iter->second;
}

auto Instr::numberOfArguments(Type t) -> unsigned int {
  switch (t) {
  case Instr::Loadc:
  case Instr::Load:
  case Instr::Store:
  case Instr::Loada:
  case Instr::Storea:
  case Instr::Jump:
  case Instr::Jumpi:
  case Instr::Jumpz:
  case Instr::Alloc: return 1;
  default: return 0;
  }
}

void Instr::print(std::span<Instr> instructions) {
  std::println("{} instructions", instructions.size());
  for (Instr i: instructions) {
    std::print("{}", Instr::toString(i.type));
    if (Instr::numberOfArguments(i.type) == 1) { std::print("\t{}", i.arg); }
    std::println();
  }
}

void CMa::step() {
  const Instr instruction = instructions[programCounter];
  programCounter += 1;
  execute(instruction);
}

auto CMa::run() -> int {
  while (programCounter < instructions.size()) { step(); }
  return EXIT_SUCCESS;
}

void CMa::execute(Instr instruction) {
  switch (instruction.type) {
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
    // TODO: what’s the semantics of this ?
    memory[stackPointer] = memory[stackPointer] ^ memory[stackPointer + 1];
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
    // TODO: Boolean negation or ones-complement?
    memory[stackPointer] = !memory[stackPointer];
  } break;

  case Instr::Load: {
    memory[stackPointer] = memory[memory[stackPointer]];
  } break;

  case Instr::Store: {
    memory[memory[stackPointer]] = memory[stackPointer - 1];
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
    stackPointer -= 1;
  } break;

  case Instr::Dup: {
    stackPointer += 1;
    memory[stackPointer] = memory[stackPointer - 1];
  } break;

  case Instr::Jump: {
    programCounter = instruction.arg;
  } break;

  case Instr::Jumpz: {
    if (memory[stackPointer] == 0) { programCounter = instruction.arg; }
    stackPointer -= 1;
  } break;

  case Instr::Jumpi: {
    programCounter = instruction.arg + memory[stackPointer];
    stackPointer -= 1;
  } break;

  case Instr::Alloc: {
    stackPointer += instruction.arg;
  } break;

  default: dbg_fail("Bad instruction", instruction.type, instruction.arg);
  }
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
    const char c = peek();
    position++;
    return c;
  }

  void consume(char c) {
    if (peek() != c) dbg_fail("Expected different char", peek());
    advance();
  }

  void skipComments() {
    consume('/');
    consume('/');
    while (peek() != '\n' && !atEnd()) { advance(); }
    consume('\n');
  }

  auto isBlank(char c) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  void skipWhiteSpace() {
    while (isBlank(peek()) && !atEnd()) { advance(); }
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
    const bool hasColon = peek() == ':';
    if (hasColon) { consume(':'); }
    skip();
    return hasColon;
  }

  auto readWord() -> std::string_view {
    skip();
    const std::size_t start = position;
    while (std::isalnum(peek()) && !atEnd()) { advance(); }
    auto w = text.substr(start, position - start);
    skip();
    return w;
  }

  auto readNumber() -> std::int32_t {
    skip();
    const std::size_t start = position;
    if (peek() == '-' || peek() == '+') { advance(); }
    while (std::isdigit(peek()) && !atEnd()) { advance(); }
    auto literal = text.substr(start, position - start);
    skip();

    std::int32_t value;
    auto result = std::from_chars(literal.begin(), literal.end(), value);
    dbg_assert_eq(result.ptr, literal.end(), "Could not parse full number",
                  literal);
    return value;
  }

  void registerLabel(std::string_view name) {
    if (mode == Mode::GatherLabels) { jumpLabels.insert({name, instr_number}); }
  }

  void handleInstruction(Instr::Type t, std::int32_t arg = 0) {
    if (mode == Mode::EmitInstructions) { instructions.push_back({t, arg}); }
    instr_number += 1;
  }

  void handleInstruction(Instr::Type t, std::string_view label) {
    if (mode == Mode::EmitInstructions) {
      const std::int32_t address = jumpLabels.at(label);
      instructions.push_back({t, address});
    }
    instr_number += 1;
  }

  void parseInstruction(std::string_view word) {
    const Instr::Type instructionType = Instr::fromString(word);
    const std::uint32_t argCount = Instr::numberOfArguments(instructionType);

    if (argCount == 1) {
      if (std::isalpha(peek())) {
        handleInstruction(instructionType, readWord());
      } else if (std::isdigit(peek())) {
        handleInstruction(instructionType, readNumber());
      } else {
        dbg_fail("bad char", peek());
      }
    } else if (argCount == 0) {
      handleInstruction(instructionType);
    } else {
      dbg_fail("bad number of arguments", argCount);
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

    for (skip(); !atEnd(); skip()) { consumeWord(); }
  }

public:
  explicit Parser(std::string_view text) :
      text {text} {}

  auto parse() -> std::vector<Instr> {
    walk(Mode::GatherLabels);
    walk(Mode::EmitInstructions);

    Instr::print(instructions);
    return std::move(instructions);
  }
};

}; // namespace

auto CMa::loadInstructions(std::string_view text) -> CMa {
  const std::vector instructions = Parser(text).parse();
  return CMa(instructions);
}

} // namespace vm::cma
