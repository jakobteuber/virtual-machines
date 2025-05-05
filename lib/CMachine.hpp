#ifndef TUM_I2_VM_LIB_C_MACHINE
#define TUM_I2_VM_LIB_C_MACHINE

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace vm::cma {

struct Instr {
  enum Type : std::uint8_t {
    Debug,
    Loadc,
    // Arithmetic and logical (as introduced in Simple expressions and
    // assignments)
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    // Comparison  (as introduced in Simple expressions and assignments)
    Eq,
    Neq,
    Le,
    Leq,
    Gr,
    Geq,
    // Negation (as introduced in Simple expressions and assignments)
    Not,
    Neg,
    // Assignments
    Load,
    Store,
    Loada,
    Storea,
    // Statements (as introduced in Statements and Statement Sequences)
    Pop,
    // Conditional and Iterative Statements
    Jump,
    Jumpz,
    // Introduced in the Switch Statement
    Jumpi,
    Dup,
    // Introduced in Storage Allocation for Variables
    Alloc
  };

  static auto toString(Type t) -> std::string_view;
  static auto fromString(std::string_view name) -> Type;
  static auto numberOfArguments(Type t) -> unsigned int;
  static void print(std::span<Instr> instructions);

  Type type;
  std::int32_t arg;
};

class CMa {
private:
  std::vector<Instr> instructions;
  std::size_t programCounter = 0;

  static constexpr std::size_t memorySize = 1ULL << 20;

  std::vector<std::int32_t> memory = std::vector<std::int32_t>(memorySize);
  std::size_t stackPointer = 0;

private:
  void debug();

public:
  explicit CMa(const std::vector<Instr>& instructions) :
      instructions {instructions} {};

  void step();
  auto run() -> int;
  void execute(Instr instruction);

  static auto loadInstructions(std::string_view name) -> CMa;
};

} // namespace vm::cma

#endif
