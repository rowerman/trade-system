#include <boost/algorithm/string.hpp>
#include <Strategy/strategyFactory.h>

namespace StarQuant {

/** @brief 根据算法名创建策略实例（工厂模式）
 *  @param _algo 策略名称，如 "SmaCross"、"SimpleCross"
 *  @return 策略实例的 unique_ptr，当前未实现分支故始终返回 nullptr
 *
 *  扩展示例：添加 SmaCross 时补充：
 *    if (boost::iequals(_algo, "smacross")) return std::make_unique<SmaCross>(); */
std::unique_ptr<StrategyBase> strategyFactory(const string& _algo) {
    // 预留：SimpleCross 已废弃，原逻辑为 boost::iequals(_algo, "simplecross") 时创建
    /*if (boost::iequals(_algo, "simplecross")) {
        return make_unique<SimpleCross>();
    }*/

    // 当前无匹配分支，返回空指针；调用方需补充 SmaCross 等策略的创建逻辑
    return nullptr;
}

}  // namespace StarQuant
