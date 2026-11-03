#ifndef __StarQuant_Strategy_StrategyFactor_H__
#define __StarQuant_Strategy_StrategyFactor_H__

#include <memory>
#include <Common/config.h>
#include <Strategy/strategybase.h>

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// strategyFactory - 策略工厂函数
//
// 功能概述：
//   根据算法名称 _algo 创建对应的 StrategyBase 派生类实例。
//   用于 StrategyManager / StrategyManagerService 按配置动态加载策略。
//
// 参数：_algo 策略算法名（如 "SmaCross"、"SimpleCross"）
// 返回：unique_ptr<StrategyBase>，当前未实现时返回 nullptr
//
// 扩展：在 cpp 中补充 if (boost::iequals(_algo, "smacross")) return make_unique<SmaCross>(); 等分支
//////////////////////////////////////////////////////////////////////////
std::unique_ptr<StrategyBase> strategyFactory(const string& _algo);

}  // namespace StarQuant

#endif  // __StarQuant_Strategy_StrategyFactor_H__
