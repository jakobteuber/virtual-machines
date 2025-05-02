#include "lib/CMachine.hpp"
#include "lib/Common.hpp"
#include "lib/Error.hpp"

#include "CMachine.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

using namespace vm::cma;

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

std::string_view Instr::toString(Type t) {
  unsigned index = static_cast<unsigned>(t);
  dbg_assert(0 <= index && index < names.size(), "Bad enum tag", t);
  return names[index];
}

Instr::Type Instr::fromString(std::string_view name) {
  std::string canonical {name};
  std::transform(canonical.begin(), canonical.end(), canonical.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  auto iter = namesToEnum.find(canonical);

  dbg_assert_neq(iter, namesToEnum.end(), "Bad enum constant", name, canonical);
  return iter->second;
}

unsigned Instr::numberOfArguments(Type t) {
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

void vm::cma::Instr::print(std::span<Instr> instructions) {
  std::println("{} instructions", instructions.size());
  for (Instr i: instructions) {
    std::print("{}", Instr::toString(i.type));
    if (Instr::numberOfArguments(i.type) == 1) { std::print("\t{}", i.arg); }
    std::println();
  }
}

void CMa::step() { dbg_fail("todo"); }

int CMa::run() {
  dbg_fail("todo");
  return 0;
}

void CMa::execute(Instr) { dbg_fail("todo"); }

namespace {

struct Parser {
  std::string_view text;
  std::size_t position = 0;
  std::int32_t instr_number = 0;
  std::vector<Instr> instructions = {};
  std::unordered_map<std::string_view, std::int32_t> jumpLabels = {};

  enum class Mode { GatherLabels, EmitInstructions };
  Mode mode = Mode::GatherLabels;

  Parser(std::string_view text) :
      text {text} {}

  void reset(Mode m) {
    mode = m;
    position = 0;
    instr_number = 0;
    instructions.clear();
  }

  bool atEnd() { return position >= text.size(); }
  char peek() { return atEnd() ? '\0' : text[position]; }

  char advance() {
    char c = peek();
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

  bool isBlank(char c) {
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

  std::string_view readWord() {
    skip();
    std::size_t start = position;
    while (std::isalnum(peek()) && !atEnd()) { advance(); }
    auto w = text.substr(start, position - start);
    return w;
  }

  std::int32_t readNumber() {
    skip();
    std::size_t start = position;
    if (peek() == '-' || peek() == '+') { advance(); }
    while (std::isdigit(peek()) && !atEnd()) { advance(); }
    auto literal = text.substr(start, position - start);

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
      std::int32_t address = jumpLabels.at(label);
      instructions.push_back({t, address});
    }
    instr_number += 1;
  }

  void parseInstruction(std::string_view word) {
    Instr::Type instructionType = Instr::fromString(word);
    std::uint32_t argCount = Instr::numberOfArguments(instructionType);

    if (argCount == 1) {
      skip();
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
    skip();
    auto word = readWord();
    skip();
    if (peek() == ':') {
      consume(':');
      registerLabel(word);
    } else {
      parseInstruction(word);
    }
  }

  std::vector<Instr> parse() {
    reset(Mode::GatherLabels);
    while (!atEnd()) { consumeWord(); }

    reset(Mode::EmitInstructions);
    while (!atEnd()) { consumeWord(); }

    Instr::print(instructions);

    return std::move(instructions);
  }
};

}; // namespace

CMa vm::cma::CMa::loadInstructions(std::string_view text) {
  std::vector instructions = Parser(text).parse();
  return CMa(instructions);
}
