cmake_minimum_required(VERSION 3.10)
project(VirtualMachines)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fdiagnostics-color=always -g")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

include("${CMAKE_SOURCE_DIR}/vendor/clang-tidy.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/Backward.cmake")

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${GTEST_INCLUDE_DIR}
)

file(GLOB_RECURSE LIB_CPP "${CMAKE_SOURCE_DIR}/lib/*.cpp")
file(GLOB_RECURSE LIB_HPP "${CMAKE_SOURCE_DIR}/lib/*.hpp")
file(GLOB_RECURSE TEST_CPP "${CMAKE_SOURCE_DIR}/test/*.cpp")
file(GLOB_RECURSE TOOLS_CPP "${CMAKE_SOURCE_DIR}/tools/*.cpp")

set(GENERATED_CPP)


# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
add_library(lib STATIC ${LIB_CPP} ${GENERATED_CPP})
target_link_libraries(lib Threads::Threads Backward::Interface)
target_compile_options(lib PRIVATE "-Werror")

add_executable(cma "${CMAKE_SOURCE_DIR}/tools/CMa.cpp")
target_link_libraries(cma lib)
target_compile_options(cma PRIVATE "-Werror")

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
add_executable(tester ${TEST_CPP})
target_link_libraries(tester lib gtest Threads::Threads)
enable_testing(tester)
add_test(lib tester)

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------
add_clang_tidy_target(src_linting "${LIB_CPP}")
add_clang_tidy_target(include_linting "${LIB_HPP}")
add_clang_tidy_target(test_linting "${TEST_CPP}")
add_clang_tidy_target(main_linting "${TOOLS_CPP}")

add_custom_target(lint)
list(APPEND lint_targets src_linting include_linting test_linting main_linting)
add_dependencies(lint ${lint_targets})

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
message(STATUS "[FILES]")
message(STATUS "    Library Files               = ${LIB_CPP}")
message(STATUS "    Library Headers             = ${LIB_HPP}")
message(STATUS "    Test Files                  = ${TEST_CPP}")
message(STATUS "    Main Executables            = ${TOOLS_CPP}")

message(STATUS "[TEST] settings")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")