#ifndef TUM_I2_VM_LIB_COMMON
#define TUM_I2_VM_LIB_COMMON

#include <concepts>
#include <string>
#include <string_view>

namespace vm::common {

template<typename VM>
concept VirtualMachine = requires(VM vm, std::string_view text) {
  { vm.step() } -> std::same_as<void>;
  { vm.run() } -> std::same_as<int>;
  { VM::loadInstructions(text) } -> std::same_as<VM>;
};

auto readFile(std::string_view name) -> std::string;

} // namespace vm::common

#endif
