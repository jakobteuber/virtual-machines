#!/bin/env python3

namespace = "Instr"
enum_name = "Type"
enum_text = """
Debug,
    Loadc,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    Eq,
    Neq,
    Le,
    Leq,
    Gr,
    Geq,
    Not,
    Neg,
    Load,
    Store,
    Loada,
    Storea,
    Pop,
    Jump,
    Jumpz,
    Jumpi,
    Dup,
    Alloc,
    New,
    Mark,
    Call,
    Slide,
    Enter,
    Return,
    Halt
"""

separator = ","

enums = [e.strip().capitalize() for e in enum_text.split(separator)]
enum_str = ['"' + e.lower() + '"' for e in enums]

print("/* Enum names: */")
print("enum", enum_name, "{", ", ".join(enums), "};")
print("\n")

print("/* Includes: */")
print('#include <array>')
print('#include <unordered_map>')
print('#include <string_view>')
print('#include <string>')
print('#include <iterator>')
print('#include <algorithms>')
print('#include <cctype>')


print("/* toString() */")
print('auto ', namespace + '::toString(', namespace +
      '::' + enum_name, 'enumValue) -> std::string_view {')
print("constexpr std::array names =", "{", ", ".join(enum_str), "};")
print('auto index = static_cast<std::size_t>(enumValue);')
print('dbg_assert(0 <= index && index < names.size(), "Bad enum tag for ' +
      namespace + '::' + enum_name + '", enumValue);')
print('return names[index];')
print('}\n')

print("/* fromString(): */")
pairs = ['{' + string + ', ' + enum_name + '::' + ident +
         '}' for (ident, string) in zip(enums, enum_str)]
print('auto ', namespace +
      '::fromString(std::string_view name) -> ', namespace + '::' + enum_name, '{')
print("static const std::unordered_map <std::string_view, ",
      namespace + '::' + enum_name, "> names = {", ', '.join(pairs), '};')
print("std::string canonical = {};")
print("std::ranges::transform(name, std::back_inserter(canonical), ",
      "[](unsigned char c) { return std::tolower(c); });")
print("auto iter = names.find(canonical);")
print('dbg_assert_neq(iter, names.end(), "Bad enum name for ' +
      namespace + '::' + enum_name + '", name, canonical);')
print('return iter->second;')
print('}\n')
