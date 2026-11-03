#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 全局变量模块

本模块提供系统级的全局存储，包括：
- dotdict：支持点号访问的字典
- context：全局上下文，用于跨模块共享运行时状态
- history_bar / history_tick：历史 K 线 / Tick 缓存
- wxcmd：微信命令（若集成微信通知）
"""

from collections import defaultdict


class dotdict(dict):
    """
    支持点号访问的字典子类。

    除常规 dict[key] 外，可使用 obj.key 读写属性，
    便于将配置或上下文以对象形式访问。

    Example:
        >>> d = dotdict({'a': 1, 'b': 2})
        >>> d.a
        1
        >>> d.c = 3
    """

    __getattr__ = dict.get
    __setattr__ = dict.__setitem__
    __delattr__ = dict.__delitem__


# 全局上下文，用于跨模块共享运行时状态（如当前引擎、策略等）
context = dotdict()


# 历史 K 线缓存：key 为合约标识，value 为 BarData 列表
history_bar = defaultdict(list)

# 历史 Tick 缓存：key 为合约标识，value 为 TickData 列表
history_tick = defaultdict(list)

# 微信命令字符串（若系统集成微信通知/控制）
wxcmd = ''
