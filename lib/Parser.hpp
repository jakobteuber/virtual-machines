#ifndef TUM_I2_VM_LIB_PARSER
#define TUM_I2_VM_LIB_PARSER

#include "lib/Error.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vm::parser {

template <typename InstrType, typename InstrByte> class Parser {
public:
  virtual auto fromString(std::string_view name) -> InstrType = 0;
  virtual auto hasArgument(InstrType t) -> bool = 0;
  virtual auto makeData(std::byte) -> InstrByte = 0;
  virtual auto makeInstr(InstrType) -> InstrByte = 0;

private:
  std::string_view text;
  std::size_t position = 0;
  std::vector<InstrByte> code = {};
  std::unordered_map<std::string_view, std::size_t> jumpLabels = {};
  std::vector<std::pair<std::size_t, std::string_view>> backpatching = {};

  auto atEnd() -> bool { return position < text.size(); }
  auto peek() -> char { return !atEnd() ? text[position] : '\0'; }
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
    jumpLabels.insert({name, code.size()});
  }

  void handleInstruction(InstrType t) { code.push_back(makeInstr(t)); }

  template <typename T>
  static auto toBytes(T value) -> std::array<std::byte, sizeof(value)> {
    std::array<std::byte, sizeof(value)> arr = {};
    std::memcpy(arr.data(), &value, sizeof(value));
    return arr;
  }

  template <typename T> void pushImmediate(T value) {
    for (std::byte b : toBytes(value)) {
      code.push_back(makeData(b));
    }
  }

  void handleInstruction(InstrType t, std::int64_t immediate) {
    code.push_back(makeInstr(t));
    pushImmediate(immediate);
  }

  void handleInstruction(InstrType t, std::string_view label) {
    code.push_back(makeInstr(t));
    auto entry = jumpLabels.find(label);
    if (entry == jumpLabels.end()) {
      backpatching.emplace_back(code.size(), label);
      pushImmediate(static_cast<std::size_t>(0));
    } else {
      std::size_t address = entry->second;
      pushImmediate(address);
    }
  }

  void patchLabels() {
    for (auto [position, label] : backpatching) {
      auto entry = jumpLabels.find(label);
      dbg_assert_neq(entry, jumpLabels.end(),
                     "label unknown while backpatching", label);
      std::size_t value = entry->second;
      std::array bytes = toBytes(value);
      // wirite bytes, check bounds
    }
  }

  void parseInstruction(std::string_view word) {
    InstrType instructionType = fromString(word);

    if (hasArgument(instructionType)) {
      if (isIdentStart(peek())) {
        handleInstruction(instructionType, readWord());
      } else if (isNumeric(peek())) {
        handleInstruction(instructionType, readNumber());
      } else {
        dbg_fail("bad char", peek());
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

public:
  explicit Parser(std::string_view text) : text{text} {}

  auto parse() -> std::vector<InstrByte> {
    for (skip(); !atEnd(); skip()) {
      consumeWord();
    }

    return std::move(code);
  }
};

} // namespace vm::parser

#endif // !TUM_I2_VM_LIB_PARSER
