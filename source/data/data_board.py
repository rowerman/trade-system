#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 数据看板模块

提供 BarGenerator（Tick -> 1 分钟 K 线 -> 多分钟/小时 K 线）
与 ArrayManager（K 线时序容器及技术指标计算）。
"""

from typing import Callable

import numpy as np
import talib

from ..common.datastruct import TickData, BarData
from ..common.constant import Interval


class BarGenerator:
    """
    K 线生成器。

    功能：
    1. 从 Tick 生成 1 分钟 K 线
    2. 从 1 分钟 K 线聚合为 x 分钟/x 小时 K 线

    注意：
    - x 分钟 K 线：x 需能整除 60，如 2、3、5、6、10、15、20、30
    - x 小时 K 线：x 可为任意正整数
    """

    def __init__(
        self,
        on_bar: Callable,
        window: int = 0,
        on_window_bar: Callable = None,
        interval: Interval = Interval.MINUTE
    ):
        """
        Args:
            on_bar: 1 分钟 K 线完成时的回调
            window: 聚合窗口（分钟数或小时数）
            on_window_bar: 聚合 K 线完成时的回调
            interval: 聚合周期（MINUTE 或 HOUR）
        """
        self.bar = None
        self.on_bar = on_bar

        self.interval = interval
        self.interval_count = 0

        if window:
            self.window = window
        else:
            self.window = 1
        self.window_bar = None
        self.on_window_bar = on_window_bar

        self.last_tick = None
        self.last_bar = None
        self.tick_counts = 0  # 分钟 K 线至少需 2 个 Tick 才触发 on_bar

    def update_tick(self, tick: TickData):
        """
        接收新 Tick，更新当前分钟 K 线或创建新 K 线。
        过滤 last_price、ask/bid 为 0 的无效 Tick。
        """
        new_minute = False

        # 过滤无效 Tick
        if not tick.last_price:
            return
        if not tick.ask_price_1 or not tick.bid_price_1:
            return

        if not self.bar:
            new_minute = True
        # 分钟或小时变化时视为新 K 线（回测换日也触发）
        elif self.bar.datetime.minute != tick.datetime.minute or self.bar.datetime.hour != tick.datetime.hour:
            self.bar.datetime = self.bar.datetime.replace(
                second=0, microsecond=0
            )
            if self.tick_counts > 5:  # 至少 5 个 Tick 才认为 K 线有效
                self.on_bar(self.bar)

            new_minute = True

        if new_minute:
            self.bar = BarData(
                symbol=tick.symbol,
                exchange=tick.exchange,
                interval=Interval.MINUTE,
                datetime=tick.datetime,
                gateway_name=tick.gateway_name,
                open_price=tick.last_price,
                high_price=tick.last_price,
                low_price=tick.last_price,
                close_price=tick.last_price,
                open_interest=tick.open_interest
            )
            self.tick_counts = 1
        else:
            if self.bar.datetime.time() != tick.datetime.time():
                self.tick_counts += 1
                self.bar.high_price = max(self.bar.high_price, tick.last_price)
                self.bar.low_price = min(self.bar.low_price, tick.last_price)
                self.bar.close_price = tick.last_price
                self.bar.open_interest = tick.open_interest
                self.bar.datetime = tick.datetime

        if self.last_tick:
            volume_change = tick.volume - self.last_tick.volume
            self.bar.volume += max(volume_change, 0)

        self.last_tick = tick

    def update_bar(self, bar: BarData):
        """
        接收 1 分钟 K 线，聚合为 window 周期的 K 线。
        完成时调用 on_window_bar。
        """
        # 未初始化时创建 window_bar
        if not self.window_bar:
            # Generate timestamp for bar data
            if self.interval == Interval.MINUTE:
                dt = bar.datetime.replace(second=0, microsecond=0)
            else:
                dt = bar.datetime.replace(minute=0, second=0, microsecond=0)

            self.window_bar = BarData(
                symbol=bar.symbol,
                exchange=bar.exchange,
                datetime=dt,
                gateway_name=bar.gateway_name,
                open_price=bar.open_price,
                high_price=bar.high_price,
                low_price=bar.low_price
            )
        # 已存在则更新 high/low
        else:
            self.window_bar.high_price = max(
                self.window_bar.high_price, bar.high_price)
            self.window_bar.low_price = min(
                self.window_bar.low_price, bar.low_price)

        # 更新 close、volume、open_interest
        self.window_bar.close_price = bar.close_price
        self.window_bar.volume += int(bar.volume)
        self.window_bar.open_interest = bar.open_interest

        # 判断 window K 线是否完成
        finished = False

        if self.interval == Interval.MINUTE:
            # x 分钟 K 线：分钟数+1 能被 window 整除时完成
            if not (bar.datetime.minute + 1) % self.window:
                finished = True
        elif self.interval == Interval.HOUR:
            if self.last_bar and bar.datetime.hour != self.last_bar.datetime.hour:
                # 1 小时 K 线
                if self.window == 1:
                    finished = True
                # x 小时 K 线
                else:
                    self.interval_count += 1

                    if not self.interval_count % self.window:
                        finished = True
                        self.interval_count = 0

        if finished:
            self.on_window_bar(self.window_bar)
            self.window_bar = None

        # 缓存上一根 K 线
        self.last_bar = bar

    def generate(self):
        """
        强制生成当前 K 线并立即回调 on_bar（如收盘时收尾）。
        """
        self.on_bar(self.bar)
        self.bar = None


class ArrayManager(object):
    """
    K 线时序容器与技术指标计算。

    维护固定长度的 OHLCV 数组，支持 SMA、RSI、MACD、布林带等指标。
    """

    def __init__(self, size=100):
        """
        Args:
            size: 数组长度，决定指标计算的最大回溯周期
        """
        self.count = 0
        self.size = size
        self.inited = False

        self.open_array = np.zeros(size)
        self.high_array = np.zeros(size)
        self.low_array = np.zeros(size)
        self.close_array = np.zeros(size)
        self.volume_array = np.zeros(size)

    def update_bar(self, bar):
        """
        追加新 K 线，左移数组并写入最新 OHLCV。
        """
        self.count += 1
        if not self.inited and self.count >= self.size:
            self.inited = True

        self.open_array[:-1] = self.open_array[1:]
        self.high_array[:-1] = self.high_array[1:]
        self.low_array[:-1] = self.low_array[1:]
        self.close_array[:-1] = self.close_array[1:]
        self.volume_array[:-1] = self.volume_array[1:]

        self.open_array[-1] = bar.open_price
        self.high_array[-1] = bar.high_price
        self.low_array[-1] = bar.low_price
        self.close_array[-1] = bar.close_price
        self.volume_array[-1] = bar.volume

    @property
    def open(self):
        """
        Get open price time series.
        """
        return self.open_array

    @property
    def high(self):
        """
        Get high price time series.
        """
        return self.high_array

    @property
    def low(self):
        """
        Get low price time series.
        """
        return self.low_array

    @property
    def close(self):
        """
        Get close price time series.
        """
        return self.close_array

    @property
    def volume(self):
        """
        Get trading volume time series.
        """
        return self.volume_array

    def sma(self, n, array=False):
        """
        简单移动平均。array=True 返回全序列，否则返回最新值。
        """
        result = talib.SMA(self.close, n)
        if array:
            return result
        return result[-1]

    def std(self, n, array=False):
        """标准差"""
        result = talib.STDDEV(self.close, n)
        if array:
            return result
        return result[-1]

    def cci(self, n, array=False):
        """商品通道指数 CCI"""
        result = talib.CCI(self.high, self.low, self.close, n)
        if array:
            return result
        return result[-1]

    def atr(self, n, array=False):
        """平均真实波幅 ATR"""
        result = talib.ATR(self.high, self.low, self.close, n)
        if array:
            return result
        return result[-1]

    def rsi(self, n, array=False):
        """相对强弱指数 RSI"""
        result = talib.RSI(self.close, n)
        if array:
            return result
        return result[-1]

    def macd(self, fast_period, slow_period, signal_period, array=False):
        """MACD 指标，返回 (macd, signal, hist)"""
        macd, signal, hist = talib.MACD(
            self.close, fast_period, slow_period, signal_period
        )
        if array:
            return macd, signal, hist
        return macd[-1], signal[-1], hist[-1]

    def adx(self, n, array=False):
        """平均趋向指数 ADX"""
        result = talib.ADX(self.high, self.low, self.close, n)
        if array:
            return result
        return result[-1]

    def boll(self, n, dev, array=False):
        """布林带，返回 (上轨, 下轨)"""
        mid = self.sma(n, array)
        std = self.std(n, array)

        up = mid + std * dev
        down = mid - std * dev

        return up, down

    def keltner(self, n, dev, array=False):
        """肯特纳通道，返回 (上轨, 下轨)"""
        mid = self.sma(n, array)
        atr = self.atr(n, array)

        up = mid + atr * dev
        down = mid - atr * dev

        return up, down

    def donchian(self, n, array=False):
        """唐奇安通道，返回 (上轨, 下轨)"""
        up = talib.MAX(self.high, n)
        down = talib.MIN(self.low, n)

        if array:
            return up, down
        return up[-1], down[-1]

    def highlow(self, currentHigh, currentLow, HighPoint, LowPoint, HigherRatio, LowerRatio):
        """
        高低点形态检测，用于识别满足条件的 A-B 结构。
        返回 (PriceBFired, PriceA, PriceB, PriceBFired2, PriceA2, PriceB2)。
        """
        PriceBFired = False
        PriceA = 0.0
        PriceB = 0.0
        PriceBFired2 = False
        PriceA2 = 0.0
        PriceB2 = 0.0

        TempHigh = np.max(self.high)
        TempLow = np.min(self.low)
        TempHighTime = np.where(self.high == TempHigh)[0][-1]  # 最后一个高点位置
        TempLowTime = np.where(self.low == TempLow)[0][0]  # 第一个低点位置
        TempLowMin1 = self.low[TempHighTime:]
        TempHighMin1 = self.high[TempLowTime:]
        if currentHigh == TempHigh and TempHigh > HighPoint:
            TempLow1 = np.min(TempLowMin1)
            TempLow1Time = np.where(TempLowMin1 == TempLow1)[0][-1]
            TempIfTimeBC = len(TempLowMin1) - TempLow1Time > 2
            TempIfTimeAB = TempLow1Time > 2
            TempIfPriceOA = (TempHigh - TempLow1 > LowerRatio *
                             HighPoint) and (TempHigh - TempLow1 < HigherRatio * HighPoint)
            if TempIfTimeBC and TempIfTimeAB and TempIfPriceOA:
                PriceBFired = True
                PriceA = TempHigh
                PriceB = TempLow1
        if currentLow == TempLow and TempLow < LowPoint:
            TempHigh1 = np.max(TempHighMin1)
            TempHigh1Time = np.where(TempHighMin1 == TempHigh1)[0][-1]
            TempIfTimeBC2 = (len(TempHighMin1) - TempHigh1Time > 2)
            TempIfTimeAB2 = (TempHigh1Time > 2)
            TempIfPriceAB2 = (TempHigh1 - TempLow > LowerRatio *
                              LowPoint) and (TempHigh1 - TempLow < HigherRatio * LowPoint)
            if TempIfTimeBC2 and TempIfTimeAB2 and TempIfPriceAB2:
                PriceBFired2 = True
                PriceA2 = TempLow
                PriceB2 = TempHigh1
        return PriceBFired, PriceA, PriceB, PriceBFired2, PriceA2, PriceB2
