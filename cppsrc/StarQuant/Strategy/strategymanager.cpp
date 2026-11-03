/*****************************************************************************
 * StrategyManager 实现文件
 *
 * 当前为空实现。StrategyManager 为占位类，策略的实际运行由
 * Services/strategyservice.cpp 中的 StrategyManagerService() 完成。
 *
 * StrategyManagerService 主循环逻辑（供参考）：
 *   1. msgq_sub_->recmsg() 接收行情，按 '|' 分割为 vs
 *   2. vs.size()==6 或 33 时解析为 Tick（fullsymbol_、time_、price_ 等）
 *   3. strategy->OnTick(k) 触发策略逻辑
 *   4. while(!msgstobrokerage.empty()) 轮询策略待发订单，msgq_pair_->sendmsg() 转发
 *   5. msgq_pair_->recmsg() 接收经纪商回报，OnGeneralMessage() 处理
 *****************************************************************************/
#include <Strategy/strategymanager.h>

namespace StarQuant {

}  // namespace StarQuant

