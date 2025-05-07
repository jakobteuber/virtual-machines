#ifndef TUM_I2_VM_LIB_C_MACHINE
#define TUM_I2_VM_LIB_C_MACHINE

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
    Alloc,
    New,
    // Funktions
    Mark,
    Call,
    Slide,
    Enter,
    Return,
    Halt
  };

  static auto toString(Type t) -> std::string_view;
  static auto fromString(std::string_view name) -> Type;

  static auto hasMandatoryArg(Type t) -> bool;
  static auto hasOptionalArg(Type t) -> bool;

  static void print(std::span<Instr> instructions);

  Type type;
  int arg;
};

class CMa {
private:
  std::vector<Instr> instructions;
  int programCounter = 0;

  static constexpr int memorySize = 1 << 20;

  std::vector<int> memory = std::vector<int>(memorySize);
  int stackPointer = 0;
  int framePointer = 0;
  int extremePointer = 0;
  int newPointer = memorySize - 1;

private:
  void debug();

public:
  explicit CMa(const std::vector<Instr>& instructions) :
      instructions {instructions} {};

  void step();
  auto run() -> int;
  void execute(Instr instruction);

  static auto loadInstructions(std::string_view text) -> CMa;
};

} // namespace vm::cma

#endif
