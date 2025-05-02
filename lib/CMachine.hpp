#ifndef TUM_I2_VM_LIB_C_MACHINE
#define TUM_I2_VM_LIB_C_MACHINE

#include <cstdint>
#include <span>
#include <string_view>

namespace vm::cma {

struct Instr {
  enum Type {
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

  static std::string_view toString(Type t);
  static Type fromString(std::string_view name);
  static unsigned numberOfArguments(Type t);
  static void print(std::span<Instr> instructions);

  Type type;
  std::int32_t arg;
};

class CMa {
  std::span<Instr> instructions;

public:
  CMa(std::span<Instr> instructions) :
      instructions {instructions} {};

  void step();
  int run();
  void execute(Instr instruction);

  static CMa loadInstructions(std::string_view name);
};

} // namespace vm::cma

#endif
