#ifndef TUM_I2_VM_LIB_PARSER
#define TUM_I2_VM_LIB_PARSER

#include "lib/Error.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vm::parser {

class WordParser {
  std::string_view text;
  std::size_t position = 0;

public:
  explicit WordParser(std::string_view text) : text{text} {}

  auto getPosition() -> std::size_t { return position; }
  auto getText() -> std::string_view { return text; }

  auto atEnd() -> bool;
  auto peek() -> char;
  auto advance() -> char;
  void consume(char c);
  void skipComments();
  auto isBlank(char c) -> bool;
  auto isNumeric(char c) -> bool;
  auto isIdentStart(char c) -> bool;
  auto isIdentPart(char c) -> bool;
  void skipWhiteSpace();
  void skip();
  auto consumeColon() -> bool;
  auto readWord() -> std::string_view;
  auto readNumber() -> std::int32_t;
};

template <typename InstrType> class BasicParser : public WordParser {
  virtual auto fromString(std::string_view) -> InstrType = 0;
  virtual auto hasArgument(InstrType) -> bool = 0;

  virtual void registerLabel(std::string_view) = 0;
  virtual void handleInstruction(InstrType) = 0;
  virtual void handleInstruction(InstrType, std::string_view) = 0;
  virtual void handleInstruction(InstrType, std::int64_t) = 0;
  virtual void patchLabels() = 0;

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
  explicit BasicParser(std::string_view text) : WordParser(text) {}

  void runParse() {
    for (skip(); !atEnd(); skip()) {
      consumeWord();
    }
    dbg_assert(atEnd(), "Could not parse full text", getPosition(),
               getText().size(), getText().substr(getPosition(), 100));
    patchLabels();
  }
};

template <typename InstrType, typename InstrByte>
class RunLengthParser : public BasicParser<InstrType> {
public:
  virtual auto makeData(std::byte) -> InstrByte = 0;
  virtual auto makeInstr(InstrType) -> InstrByte = 0;

private:
  std::vector<InstrByte> code = {};
  std::unordered_map<std::string_view, std::size_t> jumpLabels = {};
  std::vector<std::pair<std::size_t, std::string_view>> backpatching = {};

  void registerLabel(std::string_view name) override {
    jumpLabels.insert({name, code.size()});
  }

  void handleInstruction(InstrType t) override { code.push_back(makeInstr(t)); }

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

  void handleInstruction(InstrType t, std::int64_t immediate) override {
    code.push_back(makeInstr(t));
    pushImmediate(immediate);
  }

  void handleInstruction(InstrType t, std::string_view label) override {
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

  void patchLabels() override {
    for (auto [position, label] : backpatching) {
      auto entry = jumpLabels.find(label);
      dbg_assert_neq(entry, jumpLabels.end(),
                     "label unknown while backpatching", label);
      std::size_t value = entry->second;
      std::array bytes = toBytes(value);
      dbg_assert(position + bytes.size() < code.size(),
                 "Cannot patch label to this position", position, bytes.size(),
                 code.size());
      std::memcpy(code.data() + position, bytes.data(), bytes.size());
    }
  }

public:
  explicit RunLengthParser(std::string_view text)
      : BasicParser<InstrType>(text) {}

  auto parse() -> std::vector<InstrByte> {
    this->runParse();
    return std::move(code);
  }
};

} // namespace vm::parser

#endif // !TUM_I2_VM_LIB_PARSER
