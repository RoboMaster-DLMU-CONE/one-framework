#ifndef UNIT_HPP
#define UNIT_HPP
#include <cstdint>
#include <string_view>

namespace OF {
class Unit {
public:
  virtual void init() = 0;
  virtual void run() = 0;
  virtual void cleanup();

  static consteval std::string_view name() { return "Unnamed"; }
  static consteval std::string_view description() { return ""; }
  static consteval size_t stackSize() { return 1024; }
  static consteval uint8_t priority() { return 0; }

  virtual ~Unit();
};
}; // namespace OF

#endif // UNIT_HPP
