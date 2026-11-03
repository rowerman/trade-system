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

#include <Trade/calc.h>
#include <Common/datastruct.h>
#include <cmath>


using namespace std;

namespace StarQuant {
namespace Calc {

/** @brief 未实现盈亏（单笔）：多头为 LastTrade-AvgPrice，空头为 AvgPrice-LastTrade */
double OpenPT(double LastTrade, double AvgPrice, bool Side) {
    return Side ? LastTrade - AvgPrice : AvgPrice - LastTrade;
}

/** @brief 未实现盈亏（单笔）重载：PosSize=0 返回 0，否则根据正负判断多空调用上述重载 */
double OpenPT(double LastTrade, double AvgPrice, int32_t PosSize) {
    return (PosSize == 0) ? 0 : OpenPT(LastTrade, AvgPrice, PosSize > 0);
}

/** @brief 未实现盈亏（总量）：持仓数量 × (最新价 - 均价) */
double OpenPL(double LastTrade, double AvgPrice, int32_t PosSizeMultiplier) {
    return PosSizeMultiplier * (LastTrade - AvgPrice);
}

/** @brief 已实现盈亏（单笔）：多头平仓=成交价-均价，空头平仓=均价-成交价；加仓或空仓返回 0 */
double ClosePT(const Position& existing, const Fill& adjust) {
    if (existing.size_ == 0) return 0;
    if ((existing.size_ > 0) == (adjust.tradeSize_ > 0)) return 0;
    return (existing.size_ > 0) ? adjust.tradePrice_ - existing.avgPrice_ : existing.avgPrice_ - adjust.tradePrice_;
}

/** @brief 已实现盈亏（总量）：ClosePT × 实际平仓数量 × 合约乘数，平仓量取 min(|成交|,|持仓|) */
double ClosePL(const Position& existing, const Fill& adjust, int32_t multiplier) {
    int32_t closedsize = std::abs(adjust.tradeSize_) > std::abs(existing.size_) ? std::abs(existing.size_) : std::abs(adjust.tradeSize_);
    return ClosePT(existing, adjust) * closedsize * multiplier;
}

}  // namespace Calc
}  // namespace StarQuant
