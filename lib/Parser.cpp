#include "lib/Parser.hpp"
#include "lib/Error.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>

namespace vm::parser {

auto WordParser::atEnd() -> bool { return position >= text.size(); }
auto WordParser::peek() -> char { return !atEnd() ? text[position] : '\0'; }
auto WordParser::advance() -> char {
  char c = peek();
  position++;
  return c;
}

void WordParser::consume(char c) {
  if (peek() != c) {
    dbg_fail("Expected different char", peek());
  }
  advance();
}

void WordParser::skipComments() {
  consume('/');
  consume('/');
  while (peek() != '\n' && !atEnd()) {
    advance();
  }
  consume('\n');
}

auto WordParser::isBlank(char c) -> bool {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

auto WordParser::isNumeric(char c) -> bool {
  return ('0' <= c && c <= '9') || c == '-';
}

auto WordParser::isIdentStart(char c) -> bool {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

auto WordParser::isIdentPart(char c) -> bool {
  return isIdentStart(c) || isNumeric(c);
}

void WordParser::skipWhiteSpace() {
  while (isBlank(peek()) && !atEnd()) {
    advance();
  }
}

void WordParser::skip() {
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

auto WordParser::consumeColon() -> bool {
  skip();
  bool hasColon = peek() == ':';
  if (hasColon) {
    consume(':');
  }
  skip();
  return hasColon;
}

auto WordParser::readWord() -> std::string_view {
  skip();
  std::size_t start = position;
  while (isIdentPart(peek()) && !atEnd()) {
    advance();
  }
  auto w = text.substr(start, position - start);
  skip();
  return w;
}

auto WordParser::readNumber() -> std::int32_t {
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

} // namespace vm::parser
