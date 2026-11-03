#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 数据库抽象基类

定义 BaseDatabaseManager 接口及支持的数据库驱动类型（Driver）。
所有具体实现（SQL、MongoDB）需实现该接口的抽象方法。
"""

from datetime import datetime
from enum import Enum
from abc import ABC, abstractmethod
from typing import Optional, Sequence, TYPE_CHECKING


class Driver(Enum):
    """支持的数据库驱动类型"""
    SQLITE = "sqlite"
    MYSQL = "mysql"
    POSTGRESQL = "postgresql"
    MONGODB = "mongodb"


class BaseDatabaseManager(ABC):
    """
    数据库管理器抽象基类。

    定义 K 线、Tick 的加载、保存、查询最新及清理等接口，
    具体实现由 database_sql 或 database_mongo 提供。
    """

    @abstractmethod
    def load_bar_data(
        self,
        symbol: str,
        exchange: "Exchange",
        interval: "Interval",
        start: datetime,
        end: datetime
    ) -> Sequence["BarData"]:
        """加载指定时间范围内的 K 线数据"""
        pass

    @abstractmethod
    def load_tick_data(
        self,
        symbol: str,
        exchange: "Exchange",
        start: datetime,
        end: datetime
    ) -> Sequence["TickData"]:
        """加载指定时间范围内的 Tick 数据"""
        pass

    @abstractmethod
    def save_bar_data(
        self,
        datas: Sequence["BarData"],
    ):
        """保存 K 线数据"""
        pass

    @abstractmethod
    def save_tick_data(
        self,
        datas: Sequence["TickData"],
    ):
        """保存 Tick 数据"""
        pass

    @abstractmethod
    def get_newest_bar_data(
        self,
        symbol: str,
        exchange: "Exchange",
        interval: "Interval"
    ) -> Optional["BarData"]:
        """
        若有数据则返回最新一条 K 线，否则返回 None。
        """
        pass

    @abstractmethod
    def get_newest_tick_data(
        self,
        symbol: str,
        exchange: "Exchange",
    ) -> Optional["TickData"]:
        """
        若有数据则返回最新一条 Tick，否则返回 None。
        """
        pass

    @abstractmethod
    def clean(self, symbol: str):
        """删除指定合约的所有 K 线与 Tick 记录"""
        pass
