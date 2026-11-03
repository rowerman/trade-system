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

#include <Trade/portfoliomanager.h>
#include <Common/datastruct.h>

namespace StarQuant {

PortfolioManager* PortfolioManager::pinstance_ = nullptr;
mutex PortfolioManager::instancelock_;

/** @brief 析构函数（当前为空实现，positions_ 为 shared_ptr 自动释放） */
PortfolioManager::~PortfolioManager() {
}

/** @brief 获取单例实例，双重检查锁实现懒加载 */
PortfolioManager& PortfolioManager::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new PortfolioManager();
        }
    }
    return *pinstance_;
}

/** @brief 构造函数，初始化 _count 并调用 rebuild() */
PortfolioManager::PortfolioManager() : _count(0) {
    rebuild();
}

/** @brief 清空 positions_，重置 _count */
void PortfolioManager::reset() {
    /*for (auto&& p : positions_) {
        if (p.second != nullptr) delete p.second;
    }*/

    positions_.clear();
    _count = 0;
}

/** @brief 重建持仓，当前实现为调用 reset() */
void PortfolioManager::rebuild() {
    reset();
}

/** @brief 添加或更新持仓，以 pos->key_ 为键写入 positions_ */
void PortfolioManager::Add(std::shared_ptr<Position> pos) {
    if (pos)
        positions_[pos->key_] = pos;
}

/** @brief 根据成交调整持仓，当前为占位实现，直接返回 1.0，待后续接入 Calc::ClosePL 等逻辑 */
double PortfolioManager::Adjust(const Fill& fill) {
    // auto it = positions_.find(fill.fullSymbol_);
    // if (it == positions_.end()) {
    // 	Position pos;
    // 	pos.fullSymbol_ = fill.fullSymbol_;
    // 	pos.size_ = 0;
    // 	pos.avgPrice_ = 0;
    // 	positions_.insert(std::pair<string, Position>(fill.fullSymbol_, pos));

    // }

    // return positions_[fill.fullSymbol_].Adjust(fill);TODO: add adjust
    return 1.0;
}

/** @brief 按持仓键 key 查询持仓，不存在则返回 nullptr */
std::shared_ptr<Position> PortfolioManager::retrievePosition(const string& key) {
    auto it = positions_.find(key);
    if (it != positions_.end()) {
        return it->second;
    }
    return nullptr;
}


}  // namespace StarQuant
