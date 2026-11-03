/*****************************************************************************
 * Copyright [2019] 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#ifndef CPPSRC_STARQUANT_ENGINE_PAPERTDENGINE_H_
#define CPPSRC_STARQUANT_ENGINE_PAPERTDENGINE_H_

#include <Common/datastruct.h>
#include <Common/config.h>
#include <Engine/IEngine.h>

#include <memory>
#include <string>

using std::mutex;
using std::string;
using std::list;
using std::vector;

namespace StarQuant
{
//////////////////////////////////////////////////////////////////////////
// PaperTDEngine - 模拟交易引擎
//
// 功能概述：
//   纯本地模拟交易，不连接真实交易所。根据 DataManager::orderBook_ 中的行情
//   模拟成交：市价单按买卖一档成交，限价单按价格是否可成交判断。
//
// 继承关系：仅继承 IEngine，无 CTP/TAP 等 API 回调
//
// 支持订单类型：OT_Market（市价）、OT_Limit（限价）；OT_StopLimit 暂不支持
//
// 连接逻辑：connect() 直接置 estate_=LOGIN_ACK，无网络连接
//
// 依赖：DataManager（orderBook_）、OrderManager、PortfolioManager
//////////////////////////////////////////////////////////////////////////
class PaperTDEngine : public IEngine {
 public:
    string name_;            // 引擎标识，默认 "PAPER.TD"
    int64_t m_brokerOrderId_;  // 经纪商订单 ID 递增
    int64_t fillID_;         // 成交 ID 递增

    PaperTDEngine();
    ~PaperTDEngine();

    virtual void init();    // 初始化 messenger_、logger，置 estate_=DISCONNECTED
    virtual void start();   // 主循环：接收消息并处理下单/撤单/查询等
    virtual void stop();    // 置 estate_=STOP
    virtual bool connect();     // 模拟连接：msleep(1000) 后置 estate_=LOGIN_ACK
    virtual bool disconnect();  // 模拟断开：msleep(1000) 后置 estate_=DISCONNECTED

    void processbuf();  // 预留，空实现
    void timertask();   // 预留，空实现

    void insertOrder(shared_ptr<PaperOrderMsg> pmsg);     // 模拟下单并立即撮合成交
    void cancelOrder(shared_ptr<OrderActionMsg> pmsg);    // 撤单（当前未实现）
    void queryAccount(shared_ptr<MsgHeader> pmsg);        // 查询账户（当前空实现）
    void queryPosition(shared_ptr<MsgHeader> pmsg);       // 查询持仓（当前空实现）
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_ENGINE_PAPERTDENGINE_H_
