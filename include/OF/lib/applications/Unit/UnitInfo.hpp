#ifndef UNITINFO_HPP
#define UNITINFO_HPP
#include <cstdint>
#include <string_view>

namespace OF {
struct UnitInfo {
  std::string_view name;        // 单元名称
  std::string_view description; // 单元描述
  uint32_t stackSize;           // 分配的栈大小
  uint8_t priority;             // 优先级
  bool isRunning;               // 运行状态

  // 运行时统计信息（可选）
  struct RuntimeStats {
    uint32_t cpuUsage;    // CPU使用率
    uint32_t memoryUsage; // 内存使用
    uint64_t uptime;      // 运行时间
  } stats;
};
} // namespace OF

#endif // UNITINFO_HPP
