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

#ifndef CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_

#include <Common/datastruct.h>
#include <assert.h>
#include <string>
#include <numeric>
#include <mutex>
#include <regex>
#include <atomic>
#include <map>
#include <memory>

using namespace std;

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// PortfolioManager - 组合/持仓管理器（单例）
//
// 功能概述：
//   管理账户资金信息（accinfomap_）和持仓（positions_），
//   在成交回报时通过 Adjust 更新持仓，供 OrderManager::gotFill 调用。
//
// 单例模式：通过 instance() 获取唯一实例
// 协作：OrderManager::gotFill 收到成交后调用 Adjust(fill)
//////////////////////////////////////////////////////////////////////////
class PortfolioManager {
public:
    PortfolioManager();
    ~PortfolioManager();

    static PortfolioManager* pinstance_;
    static mutex instancelock_;
    static PortfolioManager& instance();

    uint64_t _count = 0;
    AccountInfo account_;                                    // 当前账户（预留）
    map<string, AccountInfo> accinfomap_;                   // 账户名 -> 账户资金信息
    map<string, std::shared_ptr<Position>> positions_;       // 持仓键 key_ -> Position
    double cash_;                                           // 现金（预留）

    void reset();                                            // 清空 positions_，重置 _count
    void rebuild();                                          // 重建持仓（当前调用 reset）

    void Add(std::shared_ptr<Position> ppos);                // 添加或更新持仓
    double Adjust(const Fill& fill);                         // 根据成交调整持仓（当前为占位实现）
    std::shared_ptr<Position> retrievePosition(const string& key);  // 按 key 查询持仓
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_
