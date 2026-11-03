#ifndef _StarQuant_Common_StraegyBase_H_
#define _StarQuant_Common_StraegyBase_H_

#include <Common/config.h>
#include <Common/util.h>
#include <Data/tick.h>
#include <Data/bar.h>
#include <Trade/order.h>
#include <Trade/fill.h>
#include <Common/logger.h>
#include <Trade/position.h>
#include <Common/msgq.h>

#include <queue>

namespace StarQuant {

struct Position;

//////////////////////////////////////////////////////////////////////////
// StrategyBase - 策略基类
//
// 功能概述：
//   所有量化策略的基类，定义行情回调（OnTick、OnBar）、订单/成交/持仓回报，
//   以及下单、撤单、订阅等出站接口。策略通过 msgstobrokerage 队列间接发送
//   消息，由 StrategyManager 服务轮询并转发给交易引擎。
//
// 子类重写：initialize、reset、teardown 及需要的 On* 回调
//////////////////////////////////////////////////////////////////////////
class StrategyBase {
public:
    bool initialized = false;           // 策略是否已初始化
    static int m_orderId;               // 静态订单 ID 计数器，SendOrder 时自增
    queue<string> msgstobrokerage;      // 待发送到经纪商的消息队列，StrategyManager 轮询转发

    StrategyBase();
    virtual ~StrategyBase();
    virtual void initialize();          // 初始化策略
    virtual void reset();               // 重置策略状态
    virtual void teardown();            // 清理资源

    // ========== 入站消息回调（子类按需重写） ==========
    virtual void OnTick(Tick& k) {}
    virtual void OnBar(Bar& k) {}
    virtual void OnOrderTicket(long oid) {}   // 订单确认
    virtual void OnOrderCancel(long oid) {}   // 撤单回报
    virtual void OnFill(Fill& f) {}           // 成交回报
    virtual void OnPosition(Position& p) {}   // 持仓回报
    virtual void OnGeneralMessage(string& msg) {}

    // ========== 出站方法 ==========
    void SendOrder(std::shared_ptr<Order> o);  // 序列化订单并 push 到 msgstobrokerage
    void SendOrderCancel(long oid);            // 发送撤单请求
    void SendSubscriptionRequest();            // 行情订阅（当前空实现）
    void SendHistoricalBarRequest();            // 历史 K 线请求（当前空实现）
    void SendGeneralInformation();             // 通用信息（当前空实现）
    void SendLog();                            // 日志（当前空实现）
};

}  // namespace StarQuant

#endif  // _StarQuant_Common_StraegyBase_H_
