#ifndef _StarQuant_Common_StrategyManager_H_
#define _StarQuant_Common_StrategyManager_H_

#include <string>
#include <sstream>
#include <map>
#include <regex>

using std::string;

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// StrategyManager - 策略管理器（占位类）
//
// 功能概述：
//   预留的策略管理模块，用于统一管理多个策略实例的增删、启停、生命周期。
//   当前为空实现，实际策略运行逻辑在 Services/strategyservice.cpp 的
//   StrategyManagerService() 函数中实现。
//
// 典型用途（规划）：
//   - 按配置加载/卸载策略（strategyFactory）
//   - 管理策略实例列表，支持多策略并行
//   - 策略启停、参数热更新
//   - 与 msgq_sub_（行情）、msgq_pair_（经纪商）的绑定与转发
//////////////////////////////////////////////////////////////////////////
class StrategyManager {
public:
    StrategyManager() = default;
    ~StrategyManager() = default;
};

}  // namespace StarQuant

#endif  // _StarQuant_Common_StrategyManager_H_