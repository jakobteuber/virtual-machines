#include <print>

int main() {
  try {
    std::println("Hello World");
    return 0;
  } catch (...) { return 1; }
}
