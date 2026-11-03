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

#ifndef CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_

#include <Common/datastruct.h>
#include <string>
#include <sstream>
#include <map>
#include <regex>
#include <mutex>
#include <atomic>
#include <memory>

using namespace std;

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// RiskManager - 风控管理器（单例）
//
// 功能概述：
//   在下单前进行风险校验，限制单笔订单数量/金额、日累计订单数/成交量/金额、
//   以及每秒订单数（流控）。
//
// 单例模式：通过 instance() 获取唯一实例
// 配置：CConfig::riskcheck、sizeperorderlimit、cashperorderlimit 等
//////////////////////////////////////////////////////////////////////////
class RiskManager {
public:
    RiskManager();
    ~RiskManager();

    static RiskManager* pinstance_;
    static mutex instancelock_;
    static RiskManager& instance();

    bool alive_;                           // 是否启用风控（来自 CConfig::riskcheck）

    int32_t limitSizePerOrder_ = 100;      // 单笔订单数量上限
    double limitCashPerOrder_ = 100000;     // 单笔订单金额上限

    int32_t limitOrderCount_ = 100;        // 日累计订单数上限
    int32_t limitCash_ = 100000;           // 日累计成交金额上限
    int32_t limitOrderSize_ = 100;         // 日累计成交量上限

    int32_t totalOrderCount_ = 0;          // 当日已下单数
    double totalCash_ = 0;                 // 当日已成交金额
    int32_t totalOrderSize_ = 0;           // 当日已成交量

    int32_t limitOrderCountPerSec_ = 10;   // 每秒订单数上限（流控）
    int32_t orderCountPerSec_ = 0;         // 当前秒内订单数

    bool passOrder(std::shared_ptr<Order>);  // 校验订单是否通过风控

    void reset();    // 从 CConfig 重新加载风控参数
    void switchday();  // 日切：清零 totalOrderCount_、totalCash_、totalOrderSize_
    void resetflow();  // 流控重置：清零 orderCountPerSec_
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_
