#include <Strategy/smacross.h>

namespace StarQuant {

/** @brief 初始化策略：order_time=-10 避免首次立即下单，buy_sell=true 表示先开多，ordercount=0，sid=1 */
void SmaCross::initialize() {
    order_time = -10;   // 时间标记，当前逻辑未使用
    buy_sell = true;    // 初始为“待开多”状态
    ordercount = 0;     // 订单计数清零
    sid = 1;            // 策略来源 ID，用于 OrderManager 按 source 查询
}

/** @brief Tick 回调：缓存行情，根据价格区间触发开多/平多，最多 5 次订单 */
void SmaCross::OnTick(Tick& k) {
    tickarray.push_back(k);  // 将当前 Tick 加入历史数组（可扩展用于计算 SMA 等）

    // 限制交易次数：ordercount<=4 时最多 5 次下单（约 2~3 个开平回合）
    if (ordercount <= 4) {

        // 开多条件：价格 < 16000 且当前为“待开多”状态
        if ((k.price_ < 16000) && (buy_sell)) {
            auto o = std::make_shared<Order>();
            o->orderType = OrderType::OT_Market;       // 市价单
            o->fullSymbol = k.fullsymbol_;            // 标的合约（与 Tick 同合约）
            o->orderSize = 1;                         // 开多 1 手
            o->orderFlag = OrderFlag::OF_OpenPosition; // 开仓
            o->source = sid;                          // 策略来源，便于撤单时按 source+clientOrderId 查询
            SendOrder(o);
            ordercount++;
            buy_sell = false;  // 切换为“待平多”，等待价格 > 15700 时平仓
            return;
        }

        // 平多条件：价格 > 15700 且当前为“待平多”状态（已有持仓）
        if ((k.price_ > 15700) && (!buy_sell)) {
            auto o_close = std::make_shared<Order>();
            o_close->source = sid;
            o_close->orderType = OrderType::OT_Market;
            o_close->fullSymbol = k.fullsymbol_;
            o_close->orderSize = -1;                      // 负数表示卖出/平仓数量
            o_close->orderFlag = OrderFlag::OF_ClosePosition;  // 平仓
            SendOrder(o_close);
            ordercount++;
            buy_sell = true;   // 切换回“待开多”，可再次开仓
            return;
        }
    }
}

/** @brief 通用消息回调：将消息打印到控制台 */
void SmaCross::OnGeneralMessage(string& msg) {
    printf("SMA strat general msg: [%s]\n", msg.c_str());
}

}  // namespace StarQuant
