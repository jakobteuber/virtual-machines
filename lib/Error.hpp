#ifndef LOXLANG_LIB_ERROR_HPP
#define LOXLANG_LIB_ERROR_HPP

#include <cstddef>
#include <string_view>

namespace jakobteuber::util::error {

/**
 * @brief Terminate the program with an error message and stacktrace.
 * @details Mostly, you don’t want to use this function directly, but rather the
 * macros defined in this header file that use these functions as backing.
 * @param msg The error message to display
 * @param expr The expression that caused the error
 * @param file The source file in which the error occurred
 * @param line The line number at which the error occurred
 */
[[noreturn]] void assertError(
    std::string_view msg, std::string_view expr, std::string_view file,
    std::size_t line
);

/**
 * @def lox_assert(cond, msg)
 * @brief Assert that the given condition hold.
 * @details If the condition does not hold, it will print an error message and a
 * stack trace and will then terminate the program.
 */
#define dbg_assert(cond, msg)                                                  \
  if (!(cond)) {                                                               \
    jakobteuber::util::error::assertError(msg, #cond, __FILE__, __LINE__);     \
  }

/**
 * @def lox_assert_eq(a, b, msg)
 * @brief Assert that `a` is equal to `b`.
 * @details If the condition does not hold, it will print an error message and a
 * stack trace and will then terminate the program.
 */
#define dbg_assert_eq(a, b, msg) lox_assert((a) == (b), msg)

/**
 * @def lox_assert_neq(a, b, msg)
 * @brief Assert that `a` is not equal to `b`.
 * @details If the condition does not hold, it will print an error message and a
 * stack trace and will then terminate the program.
 */
#define dbg_assert_neq(a, b, msg) lox_assert((a) != (b), msg)

/**
 * @def lox_fail(msg)
 * @brief Error out with a message and a stack trace.
 * @details This is meant to for catching programming errors in the interpreters
 * and debugging. This is unconnected to reporting Lox errors to the users:
 * dealing with user errors is solidly in the standard problem domain of an
 * interpreter, and should not be considered an exceptional case for the
 * interpreter.
 */
#define dbg_fail(msg)                                                          \
  jakobteuber::util::error::assertError(msg, "failure", __FILE__, __LINE__)

} // namespace jakobteuber::util::error

#endif
