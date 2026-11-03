#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 数据模块

根据配置初始化数据库管理器，支持 SQL（SQLite/MySQL/PostgreSQL）
与 NoSQL（MongoDB）两种数据存储方式。
"""

from source.common.config import get_settings
from .database import BaseDatabaseManager, Driver


def init(settings: dict) -> BaseDatabaseManager:
    """
    根据配置初始化数据库管理器。

    若 driver 为 MONGODB 则使用 NoSQL，否则使用 SQL（SQLite/MySQL/PostgreSQL）。
    """
    driver = Driver(settings["driver"])
    if driver is Driver.MONGODB:
        return init_nosql(driver=driver, settings=settings)
    else:
        return init_sql(driver=driver, settings=settings)


def init_sql(driver: Driver, settings: dict):
    """初始化 SQL 数据库管理器（SQLite/MySQL/PostgreSQL）"""
    from .database_sql import init
    keys = {'database', "host", "port", "user", "password"}
    settings = {k: v for k, v in settings.items() if k in keys}
    _database_manager = init(driver, settings)
    return _database_manager


def init_nosql(driver: Driver, settings: dict):
    """初始化 NoSQL 数据库管理器（MongoDB）"""
    from .database_mongo import init
    _database_manager = init(driver, settings=settings)
    return _database_manager


# 从配置加载 database.* 并初始化全局 database_manager
settings = get_settings("database.")
database_manager: "BaseDatabaseManager" = init(settings=settings)
