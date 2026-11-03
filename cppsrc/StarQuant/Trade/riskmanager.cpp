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


#include <Trade/riskmanager.h>
#include <Common/config.h>
#include <fmt/format.h>
#include <memory>

namespace StarQuant {

RiskManager* RiskManager::pinstance_ = nullptr;
mutex RiskManager::instancelock_;

/** @brief 构造函数，alive_=false，调用 reset() 从配置加载风控参数 */
RiskManager::RiskManager() : alive_(false) {
    // set up from config
    reset();
}

/** @brief 析构函数（当前为空实现） */
RiskManager::~RiskManager() {
}

/** @brief 获取单例实例，双重检查锁实现懒加载 */
RiskManager& RiskManager::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new RiskManager();
        }
    }
    return *pinstance_;
}

/** @brief 风控校验：若 alive_ 为 false 则直接通过；否则校验日累计、单笔、流控，全部通过返回 true */
bool RiskManager::passOrder(std::shared_ptr<Order> o) {
    if (!alive_)
        return true;
    totalOrderCount_ += 1;
    totalOrderSize_ += abs(o->quantity_);
    orderCountPerSec_ += 1;
    bool ocok = (totalOrderCount_ <= limitOrderCount_);
    bool osok = (totalOrderSize_ <= limitOrderSize_);
    bool ospok = (abs(o->quantity_) <= limitSizePerOrder_);
    bool ocpsok = (orderCountPerSec_ <= limitOrderCountPerSec_);
    if (ocok && osok && ospok && ocpsok)
        return true;
    fmt:printf("totalcount:{},totalsize{},sizeperorder:{},countpersecond{}",
        ocok, osok, ospok, ocpsok);
    return false;
}

/** @brief 从 CConfig 重新加载风控参数（alive_、limitSizePerOrder_、limitCashPerOrder_ 等） */
void RiskManager::reset() {
    alive_ = CConfig::instance().riskcheck;
    limitSizePerOrder_ = CConfig::instance().sizeperorderlimit;
    limitCashPerOrder_ = CConfig::instance().cashperorderlimit;
    limitOrderCount_ = CConfig::instance().ordercountlimit;
    limitCash_ = CConfig::instance().cashlimit;
    limitOrderSize_ = CConfig::instance().ordersizelimit;
    limitOrderCountPerSec_ = CConfig::instance().ordercountperseclimit;
}

/** @brief 流控重置：清零 orderCountPerSec_（应在每秒调用） */
void RiskManager::resetflow() {
    orderCountPerSec_ = 0;
}

/** @brief 日切：清零 totalOrderCount_、totalCash_、totalOrderSize_ */
void RiskManager::switchday() {
    totalOrderCount_ = 0;
    totalCash_ = 0.0;
    totalOrderSize_ = 0;
}

}   // namespace StarQuant

