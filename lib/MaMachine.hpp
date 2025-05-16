#ifndef TUM_I2_VM_LIB_MAMA
#define TUM_I2_VM_LIB_MAMA

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <forward_list>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace vm::mama {

namespace Instr {

enum Type : std::uint8_t {
  Debug,
  Print,
  Loadc,
  Dup,
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
  Halt,

  Jump,
  Jumpz,
  Getbasic,
  Mkbasic,

  Pushloc,
  Pushglob,

  Slide
};

union Byte {
  Type instruction;
  std::byte data;
};

auto toString(Type t) -> std::string_view;
auto fromString(std::string_view name) -> Type;
auto hasMandatoryArg(Instr::Type t) -> bool;
void print(std::span<Byte> code);

} // namespace Instr

class MaMa {
public:
  union BasicValue {
    std::int64_t value;
    std::size_t size;
    void *pointer;
  };

  struct Closure {
    std::size_t codePointer;
    std::size_t globalPointer;
  };

  struct Function {
    std::size_t codePointer;
    std::size_t argumentPointer;
    std::size_t globalPointer;
  };

  using HeapValue =
      std::variant<BasicValue, Closure, Function, std::vector<BasicValue>>;

private:
  std::span<Instr::Byte> instructions;

  static constexpr std::size_t initialStackSize = 1 << 10;
  std::vector<BasicValue> stack = std::vector<BasicValue>(initialStackSize);
  std::vector<BasicValue> gloabls = {};
  std::forward_list<HeapValue> heap = {};

  FILE *out;

public:
  explicit MaMa(std::span<Instr::Byte> instructions)
      : instructions{instructions}, out{stdout} {}
  MaMa(std::span<Instr::Byte> instructions, FILE *out)
      : instructions{instructions}, out{out} {}

  /**
   * @brief Runs the virtual machine until all instructions are executed.
   * @return Exit status of the virtual machine.
   */
  auto run() -> int;

  auto getOutFile() -> FILE * { return out; }
  auto getCodeStart() -> Instr::Byte * { return instructions.data(); }

  /**
   * @brief Prints the current state of the virtual machine for debugging.
   */
  void debug();

  auto createNew(HeapValue &&v) -> HeapValue *;

  /**
   * @brief Loads instructions from a textual representation into a CMa
   * instance.
   * @param text The textual representation of instructions.
   * @return A CMa instance with the loaded instructions.
   */
  static auto loadInstructions(std::string_view text)
      -> std::vector<Instr::Byte>;
};

} // namespace vm::mama

#endif
