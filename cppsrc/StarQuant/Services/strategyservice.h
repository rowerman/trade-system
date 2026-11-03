#ifndef __StarQuant_Engine_StrategyThread_H__
#define __StarQuant_Engine_StrategyThread_H__

#include <memory>
#include <Common/config.h>

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// StrategyManagerService - 策略管理服务
//
// 功能概述：
//   策略运行的主循环：订阅行情(msgq_sub_)、加载策略、将 Tick 推送给策略、
//   将策略的 msgstobrokerage 订单转发给交易引擎(msgq_pair_)、接收经纪商回报。
//
// 消息流：
//   行情(PUB) -> msgq_sub_(SUB) -> OnTick() -> 策略逻辑
//                                              -> msgstobrokerage
//   交易引擎 <- msgq_pair_(PAIR) <- sendmsg() <- 订单消息
//   订单回报 -> msgq_pair_(PAIR) -> OnGeneralMessage()
//////////////////////////////////////////////////////////////////////////

/** @brief 策略管理服务主函数：创建消息队列、加载策略、循环处理行情与订单 */
void StrategyManagerService();

}  // namespace StarQuant

#endif  // __StarQuant_Engine_StrategyThread_H__
