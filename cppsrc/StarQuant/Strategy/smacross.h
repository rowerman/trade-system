#ifndef _StarQuant_Strategy_SmaCross_H_
#define _StarQuant_Strategy_SmaCross_H_

#include <Strategy/strategybase.h>
#include <atomic>

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// SmaCross - 示例策略（基于价格区间的开平仓）
//
// 功能概述：
//   继承 StrategyBase，实现简单的低价开多、高价平多逻辑。
//   当 price < 16000 时开多 1 手，当 price > 15700 时平多 1 手，
//   最多执行 5 次订单（ordercount <= 4）。
//
// 注意：类名暗示均线交叉，但当前实现为价格区间策略，未使用 SMA。
//////////////////////////////////////////////////////////////////////////
class SmaCross : public StrategyBase {
public:
    virtual void initialize();              // 初始化：buy_sell=true，ordercount=0
    virtual void OnTick(Tick& k);            // Tick 回调：根据价格触发开/平仓
    virtual void OnGeneralMessage(string& msg);  // 通用消息：打印到控制台

private:
    int order_time;          // 上次下单时间标记（当前未使用）
    bool buy_sell;           // true=待开多，false=待平多
    vector<Tick> tickarray;  // 历史 Tick 缓存（可扩展用于计算均线）
    int ordercount;          // 已下单次数，<=4 时允许继续交易
    int sid;                 // 策略来源 ID，用于 Order::source
};

}  // namespace StarQuant

#endif  // _StarQuant_Strategy_SmaCross_H_
