#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 全局配置模块

本模块负责管理系统级的配置参数，包括界面字体、日志、邮件、数据源和数据库等设置。
配置采用分层覆盖机制：先加载内置默认值，再通过 JSON 文件覆盖用户自定义配置。
"""

from logging import CRITICAL
from .utility import load_json

# =============================================================================
# 全局配置字典 - 内置默认值
# =============================================================================
# 以下为系统默认配置，可通过 sq_setting.json 文件进行覆盖
# 配置项采用 "分类.键名" 的命名方式，便于按前缀分组获取
# =============================================================================

SETTINGS = {
    # ----- 界面/字体配置 -----
    "font.family": "Arial",   # 界面使用的字体族
    "font.size": 12,          # 界面字体大小（磅）

    # ----- 日志配置 -----
    "log.active": True,       # 是否启用日志功能
    "log.level": CRITICAL,    # 日志级别：DEBUG/INFO/WARNING/ERROR/CRITICAL
    "log.console": True,     # 是否输出到控制台
    "log.file": True,        # 是否写入日志文件

    # ----- 邮件配置（用于告警通知等） -----
    "email.server": "smtp.qq.com",   # SMTP 服务器地址
    "email.port": 465,               # SMTP 端口（465 为 SSL 端口）
    "email.username": "",            # 邮箱登录用户名
    "email.password": "",            # 邮箱登录密码/授权码
    "email.sender": "",              # 发件人邮箱地址
    "email.receiver": "",            # 收件人邮箱地址（告警接收方）

    # ----- 米筐数据（RQData）配置 -----
    "rqdata.username": "",    # RQData 数据源账号
    "rqdata.password": "",    # RQData 数据源密码

    # ----- 数据库配置 -----
    "database.driver": "mongodb",    # 数据库驱动类型，参见 database.Driver
    "database.database": "findata",  # 数据库名；SQLite 模式下此处为文件路径
    "database.host": "localhost",   # 数据库主机地址
    "database.port": 27017,         # 数据库端口（MongoDB 默认 27017）
    "database.user": "",            # 数据库用户名（可选）
    "database.password": "",        # 数据库密码（可选）
    "database.authentication_source": "",  # MongoDB 认证数据库（可选）
}

# =============================================================================
# 从 JSON 文件加载用户配置
# =============================================================================
# 从 sq_setting.json 读取用户自定义配置，并合并覆盖上述默认值
# 若文件不存在或解析失败，load_json 通常返回空字典，不影响默认配置
# =============================================================================

SETTING_FILENAME = "sq_setting.json"  # 用户配置文件路径（相对于工作目录）
SETTINGS.update(load_json(SETTING_FILENAME))


def get_settings(prefix: str = ""):
    """
    按前缀获取配置子集。

    从全局 SETTINGS 中筛选出以指定 prefix 开头的配置项，
    返回的字典键名会去掉前缀部分，便于模块直接使用。

    Args:
        prefix: 配置键前缀，如 "database."、"log." 等。
                传入空字符串则返回全部配置（键名保持不变）。

    Returns:
        dict: 筛选后的配置字典。键名为去掉前缀后的部分。
              例如 prefix="database." 时，"database.host" -> "host"

    Example:
        >>> get_settings("database.")
        {'driver': 'mongodb', 'database': 'findata', 'host': 'localhost', ...}
    """
    prefix_length = len(prefix)
    return {k[prefix_length:]: v for k, v in SETTINGS.items() if k.startswith(prefix)}
