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

#ifndef CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_

#include <Common/datastruct.h>
#include <Common/logger.h>
#include <string>
#include <map>
#include <mutex>
#include <atomic>

using namespace std;

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// OrderManager - 订单管理器（单例）
//
// 功能概述：
//   统一管理所有订单的生命周期，包括跟踪、报单确认、成交、撤单，
//   以及按多种条件查询订单。
//
// 单例模式：通过 instance() 获取唯一实例
// 线程安全：使用 mutex 保护 orders_、orderStatus_mtx 等
// 协作：gotFill 时调用 PortfolioManager::Adjust 更新持仓
//////////////////////////////////////////////////////////////////////////
class OrderManager {
public:
    OrderManager();
    ~OrderManager();

    static OrderManager* pinstance_;
    static mutex instancelock_;
    static OrderManager& instance();

    std::shared_ptr<SQLogger> logger;
    int32_t _count = 0;
    std::map<int64_t, std::shared_ptr<Order>> orders_;   // serverOrderID -> Order
    std::map<int64_t, int64_t> fills_;                   // serverOrderID -> 已成交数量（带符号）
    std::map<int64_t, bool> cancels_;                    // serverOrderID -> 是否已撤
    mutex wlock;
    void reset();                                        // 清空 orders_、fills_、cancels_

    void trackOrder(std::shared_ptr<Order> o);            // 将订单加入跟踪
    void gotOrder(int64_t oid);                           // 报单确认，置 OS_Acknowledged
    void gotFill(const Fill& fill);                       // 成交回报，置 OS_Filled，调用 PortfolioManager::Adjust
    void gotCancel(int64_t oid);                          // 撤单回报，置 OS_Canceled

    std::shared_ptr<Order> retrieveOrderFromServerOrderId(int64_t oid);
    std::shared_ptr<Order> retrieveOrderFromSourceAndClientOrderId(int32_t source, int64_t oid);
    std::shared_ptr<Order> retrieveOrderFromOrderNo(const string& ono);
    std::shared_ptr<Order> retrieveOrderFromAccAndBrokerOrderId(const string& acc, int32_t oid);
    std::shared_ptr<Order> retrieveOrderFromAccAndLocalNo(const string& acc, const string& ono);

    vector<std::shared_ptr<Order>> retrieveOrder(const string& fullsymbol);
    vector<std::shared_ptr<Order>> retrieveNonFilledOrderPtr();
    vector<std::shared_ptr<Order>> retrieveNonFilledOrderPtr(const string& fullsymbol);
    vector<int64_t> retrieveNonFilledOrderId();
    vector<int64_t> retrieveNonFilledOrderId(const string& fullsymbol);

    bool isEmpty();
    bool isTracked(int64_t oid);
    bool isCompleted(int64_t oid);   // 订单是否已结束（已成交或已撤）
    bool hasPendingOrders();        // 是否存在未结束的订单
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_
