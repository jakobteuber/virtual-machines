#ifndef TUM_I2_VM_LIB_C_MACHINE
#define TUM_I2_VM_LIB_C_MACHINE

#include <cstdint>
#include <cstdio>
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
    // Local Variables
    Loadrc,
    Loadr,
    Storer,
    // Whole Programs
    Halt
  };

  /**
   * @brief Converts an instruction type to its string representation.
   * @param enumValue The instruction type.
   * @return A string view of the instruction's name.
   */
  static auto toString(Type t) -> std::string_view;

  /**
   * @brief Converts a string to its corresponding instruction type.
   * @param name The string representation of the instruction.
   * @return The corresponding instruction type.
   */
  static auto fromString(std::string_view name) -> Type;

  /**
   * @brief Checks if an instruction type requires a mandatory argument.
   * @param t The instruction type.
   * @return True if the instruction requires a mandatory argument, false
   * otherwise.
   */
  static auto hasMandatoryArg(Type t) -> bool;

  /**
   * @brief Checks if an instruction type allows an optional argument.
   * @param t The instruction type.
   * @return True if the instruction allows an optional argument, false
   * otherwise.
   */
  static auto hasOptionalArg(Type t) -> bool;

  /**
   * @brief Prints a list of instructions to stderr.
   * @param instructions A span of instructions to print.
   */
  static void print(std::span<Instr> instructions);

  Type type;
  int arg;
};

class CMa {
private:
  std::span<Instr> instructions;
  int programCounter = 0;

  static constexpr int memorySize = 1 << 20;

  std::vector<int> memory = std::vector<int>(memorySize);
  int stackPointer = 0;
  int framePointer = 0;
  int extremePointer = 0;
  int newPointer = memorySize - 1;

  FILE* out;

private:
  /**
   * @brief Prints the current state of the virtual machine for debugging.
   */
  void debug();

public:
  explicit CMa(std::span<Instr> instructions) :
      instructions {instructions},
      out {stdout} {}
  CMa(std::span<Instr> instructions, FILE* out) :
      instructions {instructions},
      out {out} {}

  /**
   * @brief Executes a single instruction and advances the program counter.
   */
  void step();

  /**
   * @brief Runs the virtual machine until all instructions are executed.
   * @return Exit status of the virtual machine.
   */
  auto run() -> int;

  /**
   * @brief Executes a single instruction based on its type.
   * @param instruction The instruction to execute.
   */
  void execute(Instr instruction);

  /**
   * @brief Loads instructions from a textual representation into a CMa
   * instance.
   * @param text The textual representation of instructions.
   * @return A CMa instance with the loaded instructions.
   */
  static auto loadInstructions(std::string_view text) -> std::vector<Instr>;
};

} // namespace vm::cma

#endif
