#ifndef CPPSRC_STARQUANT_TRADE_CALC_H
#define CPPSRC_STARQUANT_TRADE_CALC_H

#include <Common/datastruct.h>

namespace StarQuant {
//////////////////////////////////////////////////////////////////////////
// Calc - 盈亏计算工具
//
// 功能概述：
//   提供持仓未实现盈亏（Open PL）和已实现盈亏（Close PL）的计算函数，
//   支持按单笔（per-tick/share）和按持仓总量两种计算方式。
//
// 依赖：Position（持仓）、Fill（成交）来自 Common/datastruct.h
//////////////////////////////////////////////////////////////////////////
namespace Calc {
    /** @brief 未实现盈亏（单笔）：按每单位计算，不考虑持仓数量
     *  @param LastTrade 最新成交价
     *  @param AvgPrice 持仓均价
     *  @param Side true=多头，false=空头
     *  @return 多头：(LastTrade-AvgPrice)，空头：(AvgPrice-LastTrade) */
    double OpenPT(double LastTrade, double AvgPrice, bool Side);

    /** @brief 未实现盈亏（单笔）重载：根据持仓数量符号自动判断多空
     *  @param PosSize 持仓数量，>0 多头，<0 空头，=0 返回 0 */
    double OpenPT(double LastTrade, double AvgPrice, int32_t PosSize);

    /** @brief 未实现盈亏（总量）：考虑持仓数量
     *  @param PosSizeMultiplier 持仓数量（正=多，负=空）
     *  @return PosSizeMultiplier * (LastTrade - AvgPrice) */
    double OpenPL(double LastTrade, double AvgPrice, int32_t PosSizeMultiplier);

    /** @brief 已实现盈亏（单笔）：按每单位计算，不考虑平仓数量
     *  @param existing 原持仓
     *  @param adjust 平仓成交
     *  @return 多头平仓：(成交价-均价)，空头平仓：(均价-成交价)；加仓或空仓返回 0 */
    double ClosePT(const Position& existing, const Fill& adjust);

    /** @brief 已实现盈亏（总量）：按实际平仓数量计算
     *  @param multiplier 合约乘数，默认 1
     *  @return ClosePT * min(|平仓量|,|持仓量|) * multiplier */
    double ClosePL(const Position& existing, const Fill& adjust, int32_t multiplier = 1);
}  // namespace Calc
}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_CALC_H
