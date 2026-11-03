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

#include <Trade/ordermanager.h>
#include <Trade/portfoliomanager.h>
#include <Common/logger.h>
#include <Common/datastruct.h>

namespace StarQuant {

OrderManager* OrderManager::pinstance_ = nullptr;
mutex OrderManager::instancelock_;

/** @brief 构造函数，初始化 _count 并调用 reset()，创建 SYS 日志器 */
OrderManager::OrderManager() : _count(0) {
    // set up stocks from config
    reset();
    logger = SQLogger::getLogger("SYS");
}

/** @brief 析构函数（当前为空实现） */
OrderManager::~OrderManager() {
}

/** @brief 获取单例实例，双重检查锁实现懒加载 */
OrderManager& OrderManager::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new OrderManager();
        }
    }
    return *pinstance_;
}

/** @brief 清空 orders_、fills_、cancels_，重置 _count */
void OrderManager::reset() {
    orders_.clear();
    fills_.clear();
    cancels_.clear();

    _count = 0;
}

/** @brief 将订单加入跟踪：若已存在则跳过，否则写入 orders_、cancels_、fills_ */
void OrderManager::trackOrder(std::shared_ptr<Order> o) {
    // if (o->orderSize_ == 0) {
    //  LOG_ERROR(logger, "Incorrect OrderSize.");
    //  return;
    // }

    auto iter = orders_.find(o->serverOrderID_);
    if (iter != orders_.end())  // order exists
        return;

    orders_[o->serverOrderID_] = o;  // add to map
    cancels_[o->serverOrderID_] = false;
    fills_[o->serverOrderID_] = 0;
    LOG_INFO(logger, "Order is put under track. ServerOrderId=" << o->serverOrderID_);
}

/** @brief 报单确认：若订单在跟踪中且状态为 NewBorn/Submitted，则置为 OS_Acknowledged */
void OrderManager::gotOrder(int64_t oid) {
    if (!isTracked(oid)) {
        LOG_ERROR(logger, "Order is not tracked. ServerOrderId= " << oid);
        return;
    }

    lock_guard<mutex> g(orderStatus_mtx);
    if ((orders_[oid]->orderStatus_ == OrderStatus::OS_NewBorn)
        || (orders_[oid]->orderStatus_ == OrderStatus::OS_Submitted)) {
        orders_[oid]->orderStatus_ = OrderStatus::OS_Acknowledged;
    }
}

/** @brief 成交回报：置订单状态为 OS_Filled，更新 updateTime_，调用 PortfolioManager::Adjust */
void OrderManager::gotFill(const Fill& fill) {
    if (!isTracked(fill.serverOrderID_)) {
        LOG_ERROR(logger, "Order is not tracked. ServerOrderId= " << fill.serverOrderID_);
    } else {
        LOG_INFO(logger,"Order is filled. ServerOrderId="<<fill.serverOrderID_<<"price = "<<fill.tradePrice_);
        lock_guard<mutex> g(orderStatus_mtx);
        orders_[fill.serverOrderID_]->orderStatus_ = OrderStatus::OS_Filled;
        orders_[fill.serverOrderID_]->updateTime_ = ymdhmsf();
        // TODO: check for partial fill
        PortfolioManager::instance().Adjust(fill);
    }
}

/** @brief 撤单回报：置订单状态为 OS_Canceled，更新 updateTime_，cancels_[oid]=true */
void OrderManager::gotCancel(int64_t oid) {
    if (isTracked(oid)) {
        lock_guard<mutex> g(orderStatus_mtx);
        orders_[oid]->orderStatus_ = OrderStatus::OS_Canceled;
        orders_[oid]->updateTime_ = ymdhmsf();
        cancels_[oid] = true;
    }
}

/** @brief 按 serverOrderID 查询订单 */
std::shared_ptr<Order> OrderManager::retrieveOrderFromServerOrderId(int64_t oid) {
    if (orders_.count(oid)) {
        return orders_[oid];
    }
    return nullptr;
}

/** @brief 按 clientID + clientOrderID 查询订单 */
std::shared_ptr<Order> OrderManager::retrieveOrderFromSourceAndClientOrderId(int32_t source, int64_t oid) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->clientOrderID_ == oid) && (iterator->second->clientID_ == source)) {
            return iterator->second;
        }
    }

    return nullptr;
}

/** @brief 按交易所订单号 orderNo 查询订单 */
std::shared_ptr<Order> OrderManager::retrieveOrderFromOrderNo(const string& ono) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->orderNo_ == ono)) {
            return iterator->second;
        }
    }

    return nullptr;
}

/** @brief 按账户 + brokerOrderID 查询订单 */
std::shared_ptr<Order> OrderManager::retrieveOrderFromAccAndBrokerOrderId(const string& acc, int32_t oid) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->brokerOrderID_ == oid) && (iterator->second->account_ == acc)) {
            return iterator->second;
        }
    }
    return nullptr;
}

/** @brief 按账户 + localNo 查询订单 */
std::shared_ptr<Order> OrderManager::retrieveOrderFromAccAndLocalNo(const string& acc, const string& ono) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->localNo_ == ono) && (iterator->second->account_ == acc)) {
            return iterator->second;
        }
    }
    return nullptr;
}

// std::shared_ptr<Order> OrderManager::retrieveOrderFromMatchNo(string fno) {
// 	for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
// 		if ((iterator->second->fillNo_ == fno))
// 		{
// 			return iterator->second;
// 		}
// 	}

// 	return nullptr;
// }

/** @brief 按合约 fullsymbol 查询所有匹配订单 */
vector<std::shared_ptr<Order>> OrderManager::retrieveOrder(const string& fullsymbol) {
    vector<std::shared_ptr<Order>> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (iterator->second->fullSymbol_ == fullsymbol) {
            v.push_back(iterator->second);
        }
    }

    return v;
}

/** @brief 获取所有活跃订单（isActiveOS 为 true）的 Order 指针列表 */
vector<std::shared_ptr<Order>> OrderManager::retrieveNonFilledOrderPtr() {
    vector<std::shared_ptr<Order>> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (isActiveOS(iterator->second->orderStatus_)) {
            v.push_back(iterator->second);
        }
    }

    return v;
}

/** @brief 获取指定合约的活跃订单 Order 指针列表 */
vector<std::shared_ptr<Order>> OrderManager::retrieveNonFilledOrderPtr(const string& fullsymbol) {
    vector<std::shared_ptr<Order>> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ( isActiveOS(iterator->second->orderStatus_)  && (iterator->second->fullSymbol_ == fullsymbol)) {
            v.push_back(iterator->second);
        }
    }

    return v;
}

/** @brief 获取所有活跃订单的 serverOrderID 列表 */
vector<int64_t> OrderManager::retrieveNonFilledOrderId() {
    vector<int64_t> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (isActiveOS(iterator->second->orderStatus_)) {
            v.push_back(iterator->first);
        }
    }

    return v;
}

/** @brief 获取指定合约的活跃订单 serverOrderID 列表 */
vector<int64_t> OrderManager::retrieveNonFilledOrderId(const string& fullsymbol) {
    vector<int64_t> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((isActiveOS(iterator->second->orderStatus_)) && (iterator->second->fullSymbol_ == fullsymbol)) {
            v.push_back(iterator->first);
        }
    }

    return v;
}

/** @brief 判断 orders_ 是否为空 */
bool OrderManager::isEmpty() {
    return orders_.empty();
}

/** @brief 判断订单是否在跟踪列表中 */
bool OrderManager::isTracked(int64_t oid) {
    auto it = orders_.find(oid);
    return (it != orders_.end());
}

/** @brief 判断订单是否已结束（已成交或已撤），未跟踪时返回 true */
bool OrderManager::isCompleted(int64_t oid) {
    if (isTracked(oid)) {
        return isActiveOS(orders_[oid]->orderStatus_);
    } else {
        LOG_ERROR(logger, "Order is not tracked. ServerOrderId= " << oid);
        return true;
    }
}

/** @brief 判断是否存在未结束的订单（isActiveOS 为 true） */
bool OrderManager::hasPendingOrders() {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (isActiveOS(iterator->second->orderStatus_)) {
            return true;
        }
    }
    return false;
}

}  // namespace StarQuant
