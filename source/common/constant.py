#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 常量定义模块

本模块集中定义量化交易系统所需的各类枚举常量，包括：
- 引擎/连接状态、消息类型、事件类型
- 订单方向、开平标志、订单类型、订单状态
- 行情数据类型、交易所、产品类型等
- 与 CTP 接口的映射关系
"""

from enum import Enum
from ..api.ctp_constant import (
    THOST_FTDC_D_Buy,
    THOST_FTDC_D_Sell,
    THOST_FTDC_PD_Long,
    THOST_FTDC_PD_Short,
    THOST_FTDC_PD_Net
)


# =============================================================================
# 引擎/连接状态
# =============================================================================

class ESTATE(Enum):
    """
    引擎连接状态枚举。

    描述交易/行情引擎与交易所/经纪商的连接生命周期。
    """
    DISCONNECTED = 0      # 已断开
    CONNECTING = 1        # 连接中
    CONNECT_ACK = 2       # 连接确认
    AUTHENTICATING = 3    # 认证中
    AUTHENTICATE_ACK = 4  # 认证确认
    LOGINING = 5          # 登录中
    LOGIN_ACK = 6         # 登录确认
    LOGOUTING = 7         # 登出中
    STOP = 8              # 已停止


class SYMBOL_TYPE(Enum):
    """
    合约代码格式类型。

    用于区分 full_symbol（完整格式）与 CTP 原始格式。
    """
    FULL = 0   # full_symbol 格式，如 "SHFE F au 2412"
    CTP = 1    # CTP 原始格式，如 "au2412"


class MSG_TYPE(Enum):
    """
    消息类型枚举。

    系统内部消息传递的类型标识，按百位分组：
    10* 行情数据，11* 系统控制，12* 策略，13* 任务，14* 录制器，
    20* 引擎请求，25* 回调，31* 信息，34* 错误，40* 测试
    """
    # ----- 10* 行情数据 -----
    MSG_TYPE_TICK = 1000       # 通用 Tick
    MSG_TYPE_TICK_L1 = 1001    # L1 档位 Tick
    MSG_TYPE_TICK_L5 = 1002    # L5 档位 Tick
    MSG_TYPE_TICK_L10 = 1003   # L10 档位 Tick
    MSG_TYPE_TICK_L20 = 1004   # L20 档位 Tick
    MSG_TYPE_BAR = 1010        # 通用 K 线
    MSG_TYPE_BAR_1MIN = 1011   # 1 分钟 K 线
    MSG_TYPE_BAR_5MIN = 1012   # 5 分钟 K 线
    MSG_TYPE_BAR_15MIN = 1013  # 15 分钟 K 线
    MSG_TYPE_BAR_1HOUR = 1014  # 1 小时 K 线
    MSG_TYPE_BAR_1DAY = 1015   # 日 K 线
    MSG_TYPE_BAR_1WEEK = 1016  # 周 K 线
    MSG_TYPE_BAR_1MON = 1017   # 月 K 线
    MSG_TYPE_STOCK_TICK = 1020 # 股票 Tick
    MSG_TYPE_STOCK_BAR = 1021  # 股票 K 线
    MSG_TYPE_Trade = 1060      # 成交
    MSG_TYPE_Bid = 1061        # 买价
    MSG_TYPE_Ask = 1062        # 卖价
    MSG_TYPE_Full = 1063       # 全档行情
    MSG_TYPE_BidPrice = 1064   # 买一价
    MSG_TYPE_BidSize = 1065    # 买一量
    MSG_TYPE_AskPrice = 1066   # 卖一价
    MSG_TYPE_AskSize = 1067    # 卖一量
    MSG_TYPE_TradePrice = 1068 # 成交价
    MSG_TYPE_TradeSize = 1069  # 成交量
    MSG_TYPE_OpenPrice = 1070  # 开盘价
    MSG_TYPE_HighPrice = 1071  # 最高价
    MSG_TYPE_LowPrice = 1072   # 最低价
    MSG_TYPE_ClosePrice = 1073 # 收盘价
    MSG_TYPE_Volume = 1074     # 成交量
    MSG_TYPE_OpenInterest = 1075  # 持仓量
    MSG_TYPE_Hist = 1076      # 历史数据
    # ----- 11* 系统控制 -----
    MSG_TYPE_ENGINE_STATUS = 1101      # 引擎状态
    MSG_TYPE_ENGINE_START = 1111      # 启动引擎
    MSG_TYPE_ENGINE_STOP = 1112       # 停止引擎
    MSG_TYPE_ENGINE_RESET = 1113      # 重置引擎
    MSG_TYPE_ENGINE_CONNECT = 1120    # 连接引擎
    MSG_TYPE_ENGINE_DISCONNECT = 1121 # 断开引擎
    MSG_TYPE_SWITCH_TRADING_DAY = 1141 # 切换交易日
    # ----- 12* 策略控制 -----
    MSG_TYPE_STRATEGY_STATUS = 1200   # 策略状态
    MSG_TYPE_STRATEGY_ADD = 1210      # 添加策略
    MSG_TYPE_STRATEGY_INIT = 1211     # 初始化策略
    MSG_TYPE_STRATEGY_INIT_ALL = 1212 # 初始化全部策略
    MSG_TYPE_STRATEGY_START = 1213    # 启动策略
    MSG_TYPE_STRATEGY_START_ALL = 1214  # 启动全部策略
    MSG_TYPE_STRATEGY_STOP = 1215     # 停止策略
    MSG_TYPE_STRATEGY_STOP_ALL = 1216 # 停止全部策略
    MSG_TYPE_STRATEGY_RESET = 1217    # 重置策略
    MSG_TYPE_STRATEGY_RESET_ALL = 1218  # 重置全部策略
    MSG_TYPE_STRATEGY_RELOAD = 1219   # 重载策略
    MSG_TYPE_STRATEGY_EDIT = 1220     # 编辑策略
    MSG_TYPE_STRATEGY_REMOVE = 1221   # 移除策略
    MSG_TYPE_STRATEGY_REMOVE_DUPLICATE = 1222  # 移除重复策略
    MSG_TYPE_STRATEGY_RTN_REMOVE = 1223  # 移除策略回报
    MSG_TYPE_STRATEGY_GET_DATA = 1230     # 获取策略数据
    MSG_TYPE_STRATEGY_RTN_DATA = 1231     # 策略数据回报
    MSG_TYPE_STRATEGY_GET_CLASS_NAME = 1232    # 获取策略类名
    MSG_TYPE_STRATEGY_RTN_CLASS_NAME = 1233    # 策略类名回报
    MSG_TYPE_STRATEGY_GET_CLASS_PARAMETERS = 1234  # 获取策略类参数
    MSG_TYPE_STRATEGY_RTN_CLASS_PARAMETERS = 1235  # 策略类参数回报
    MSG_TYPE_STRATEGY_GET_PARAMETERS = 1234   # 获取策略参数
    MSG_TYPE_STRATEGY_RTN_PARAMETERS = 1235  # 策略参数回报
    # ----- 13* 任务 -----
    MSG_TYPE_TIMER = 1301       # 定时器
    MSG_TYPE_TASK_START = 1310  # 启动任务
    MSG_TYPE_TASK_STOP = 1311   # 停止任务
    # ----- 14* 录制器 -----
    MSG_TYPE_RECORDER_STATUS = 1400   # 录制器状态
    MSG_TYPE_RECORDER_START = 1401    # 启动录制
    MSG_TYPE_RECORDER_STOP = 1402     # 停止录制
    MSG_TYPE_RECORDER_RESET = 1403    # 重置录制器
    MSG_TYPE_RECORDER_RELOAD = 1404   # 重载录制器
    MSG_TYPE_RECORDER_GET_DATA = 1405 # 获取录制数据
    MSG_TYPE_RECORDER_RTN_DATA = 1406 # 录制数据回报
    MSG_TYPE_RECORDER_ADD_TICK = 1410 # 添加 Tick 录制
    MSG_TYPE_RECORDER_ADD_BAR = 1420  # 添加 K 线录制
    MSG_TYPE_RECORDER_REMOVE_TICK = 1430  # 移除 Tick 录制
    MSG_TYPE_RECORDER_REMOVE_BAR = 1440   # 移除 K 线录制
    # ----- 20* 引擎请求 -----
    MSG_TYPE_SUBSCRIBE_MARKET_DATA = 2001   # 订阅行情
    MSG_TYPE_SUBSCRIBE_L2_MD = 2002        # 订阅 L2 行情
    MSG_TYPE_SUBSCRIBE_INDEX = 2003        # 订阅指数
    MSG_TYPE_SUBSCRIBE_ORDER_TRADE = 2004  # 订阅委托成交
    MSG_TYPE_UNSUBSCRIBE = 2011            # 取消订阅
    MSG_TYPE_QRY_COMMODITY = 2021          # 查询品种
    MSG_TYPE_QRY_CONTRACT = 2022           # 查询合约
    MSG_TYPE_QRY_POS = 2023                # 查询持仓
    MSG_TYPE_QRY_ACCOUNT = 2024            # 查询账户
    MSG_TYPE_QRY_ORDER = 2025              # 查询委托
    MSG_TYPE_QRY_TRADE = 2026              # 查询成交
    MSG_TYPE_QRY_POSDETAIL = 2027          # 查询持仓明细
    MSG_TYPE_ORDER = 2030                  # 下单（通用）
    MSG_TYPE_ORDER_PAPER = 2031    # 模拟盘下单
    MSG_TYPE_ORDER_CTP = 2032     # CTP 下单
    MSG_TYPE_ORDER_CTP_PARKED = 2033  # CTP 预埋单
    MSG_TYPE_ORDER_TAP = 2034     # TAP 下单
    MSG_TYPE_ORDER_XTP = 2035     # XTP 下单
    MSG_TYPE_ORDER_ACTION = 2040  # 撤单（通用）
    MSG_TYPE_CANCEL_ORDER = 2041   # 撤销指定委托
    MSG_TYPE_CANCEL_ALL = 2042     # 撤销全部委托
    MSG_TYPE_ORDER_ACTION_CTP = 2043  # CTP 撤单
    MSG_TYPE_ORDER_ACTION_TAP = 2044  # TAP 撤单
    MSG_TYPE_ORDER_ACTION_XTP = 2045  # XTP 撤单
    # ----- 25* 回调 -----
    MSG_TYPE_RSP_POS = 2500        # 持仓查询回报
    MSG_TYPE_RTN_ORDER = 2510      # 委托回报（通用）
    MSG_TYPE_RTN_ORDER_CTP = 2511  # CTP 委托回报
    MSG_TYPE_RTN_ORDER_TAP = 2512  # TAP 委托回报
    MSG_TYPE_RTN_ORDER_XTP = 2513  # XTP 委托回报
    MSG_TYPE_RTN_TRADE = 2520      # 成交回报（通用）
    MSG_TYPE_RTN_TRADE_CTP = 2521  # CTP 成交回报
    MSG_TYPE_RTN_TRADE_TAP = 2522  # TAP 成交回报
    MSG_TYPE_RTN_TRADE_XTP = 2523  # XTP 成交回报
    MSG_TYPE_RSP_ACCOUNT = 2530    # 账户查询回报
    MSG_TYPE_RSP_CONTRACT = 2540   # 合约查询回报
    MSG_TYPE_RSP_COMMODITY = 2541   # 品种查询回报
    # ----- 31* 系统信息 -----
    MSG_TYPE_INFO = 3100                    # 通用信息
    MSG_TYPE_INFO_ENGINE_MDCONNECTED = 3101  # 行情引擎已连接
    MSG_TYPE_INFO_ENGINE_MDDISCONNECTED = 3102  # 行情引擎已断开
    MSG_TYPE_INFO_ENGINE_TDCONNECTED = 3103  # 交易引擎已连接
    MSG_TYPE_INFO_ENGINE_TDDISCONNECTED = 3104  # 交易引擎已断开
    MSG_TYPE_INFO_HEARTBEAT_WARNING = 3105  # 心跳告警
    MSG_TYPE_INFO_ENGINE_STATUS = 3106       # 引擎状态信息
    # ----- 34* 错误 -----
    MSG_TYPE_ERROR = 3400                    # 通用错误
    MSG_TYPE_ERROR_ENGINENOTCONNECTED = 3401 # 引擎未连接
    MSG_TYPE_ERROR_SUBSCRIBE = 3402           # 订阅错误
    MSG_TYPE_ERROR_INSERTORDER = 3403        # 下单错误
    MSG_TYPE_ERROR_CANCELORDER = 3404        # 撤单错误
    MSG_TYPE_ERROR_ORGANORDER = 3405         # 订单未被 OrderManager 跟踪
    MSG_TYPE_ERROR_QRY_ACC = 3406            # 账户查询错误
    MSG_TYPE_ERROR_QRY_POS = 3407            # 持仓查询错误
    MSG_TYPE_ERROR_QRY_CONTRACT = 3408       # 合约查询错误
    MSG_TYPE_ERROR_CONNECT = 3409            # 连接/登录失败
    MSG_TYPE_ERROR_DISCONNECT = 3410         # 断开错误
    MSG_TYPE_ERROR_NOACCOUNT = 3411         # 无账户
    # ----- 40* 测试 -----
    MSG_TYPE_TEST = 4000
    MSG_TYPE_BASE = 9


class EventType(Enum):
    """
    事件类型枚举。

    用于事件总线/消息分发时区分事件类别，与 MSG_TYPE 部分对应。
    """
    HEADER = 0           # 消息头
    TICK = 1000           # Tick 行情
    BAR = 1011            # K 线
    HISTORICAL = 1076     # 历史数据
    ENGINE_CONTROL = 1100 # 引擎控制
    STRATEGY_CONTROL = 1200  # 策略控制
    TIMER = 1301          # 定时器
    RECORDER_CONTROL = 1400  # 录制器控制
    GENERAL_REQ = 2000    # 通用请求
    SUBSCRIBE = 2001      # 订阅
    UNSUBSCIRBE = 2011    # 取消订阅（注：拼写为 UNSUBSCIRBE）
    QRY = 2020            # 查询
    ORDER = 2031          # 下单
    CANCEL = 2033         # 撤单
    ORDERSTATUS = 2052    # 委托状态
    FILL = 2053           # 成交
    ACCOUNT = 2054        # 账户
    POSITION = 2051       # 持仓
    CONTRACT = 2055       # 合约
    INFO = 3100            # 信息
    ERROR = 3400           # 错误
    BACKTEST_START = 9000  # 回测开始
    BACKTEST_FINISH = 9001 # 回测结束
    OPTIMIZATION_START = 9002  # 优化开始
    OPTIMIZATION_FINISH = 9003 # 优化结束
    BACKTEST_LOG = 9010   # 回测日志


class OrderFlag(Enum):
    """
    开平标志枚举。

    用于区分订单的开仓/平仓类型，期货中平仓需区分平今/平昨。
    """
    OPEN = 0              # 开仓
    CLOSE = 1             # 平仓（不区分今昨）
    CLOSE_TODAY = 2       # 平今
    CLOSE_YESTERDAY = 3   # 平昨
    FORCECLOSE = 4        # 强平
    FORCEOFF = 5          # 强减
    LOCALFORCECLOSE = 6   # 本地强平


class OrderType(Enum):
    """
    订单类型枚举。

    支持市价、限价、止损、FOK/FAK 等多种下单方式。
    """
    MKT = 0           # 市价单
    MKTC = 1          # 收盘市价（market on close）
    LMT = 2           # 限价单
    LMTC = 3          # 收盘限价
    PTM = 4           # 盯市单（pegged to market）
    STP = 5            # 市价止损
    STPLMT = 6        # 限价止损
    TRAIING_STOP = 7   # 跟踪止损（trailing stop）
    REL = 8            # 相对价格单（relative）
    VWAP = 9           # 成交量加权平均价（VWAP）
    TSL = 10           # 跟踪止损限价（trailing stop limit）
    VLT = 11           # 波动率单（volatility）
    NONE = 12          # 无
    EMPTY = 13         # 空
    DEFAULT = 14       # 默认
    SCALE = 15         # 分批单
    MKTT = 16          # 触及市价（market if touched）
    LMTT = 17          # 触及限价（limit if touched）
    OPTE = 18          # TAP 期权执行
    OPTA = 19          # 期权放弃
    REQQ = 20          # 询价请求（request quote）
    RSPQ = 21          # 询价响应（response quote）
    SWAP = 22          # 互换
    FAK = 23           # Fill and Kill，立即成交剩余撤销
    FOK = 24           # Fill or Kill，全部成交或全部撤销
    LPT = 25           # 本地条件单（local price condition touched）


# 订单类型到中文显示的映射，用于 UI 展示
OT2STR = {
    OrderType.MKT: '市价',
    OrderType.LMT: '限价',
    OrderType.STP: '市价止损',
    OrderType.STPLMT: '限价止损',
    OrderType.FAK: 'FAK',
    OrderType.FOK: 'FOK',
    OrderType.LPT: '本地条件单',
    OrderType.DEFAULT: '未知'
}


class OrderStatus(Enum):
    """
    订单状态枚举。

    描述订单从创建到成交/撤销的完整生命周期。
    """
    UNKNOWN = 0          # 未知
    NEWBORN = 1          # 新建，待提交
    PENDING_SUBMIT = 2   # 待提交
    SUBMITTED = 3        # 已提交
    ACKNOWLEDGED = 4     # 已确认
    QUEUED = 5           # 已排队
    PARTIALLY_FILLED = 6 # 部分成交
    FILLED = 7           # 全部成交
    PENDING_CANCEL = 8   # 待撤销
    PENDING_MODIFY = 9   # 待修改
    CANCELED = 10        # 已撤销
    LEFTDELETE = 11      # 剩余已删除
    SUSPENDED = 12       # 挂起
    API_PENDING = 13     # API 待处理
    API_CANCELLED = 14   # API 已撤销
    FAIL = 15            # 失败
    DELETED = 16         # 已删除
    EFFECT = 17          # 已生效
    APPLY = 18           # 申请中
    ERROR = 19           # 错误/拒单
    TRIG = 20            # 已触发
    EXCTRIG = 21         # 已执行触发


class TickType(Enum):
    """
    行情数据类型枚举。

    与 MSG_TYPE 对应，用于标识 Tick/K 线中的具体字段类型。
    """
    Tick_L1 = 1000    # L1 档 Tick
    Tick_L5 = 1001    # L5 档 Tick
    Tick_L10 = 1002   # L10 档 Tick
    Tick_L20 = 1003   # L20 档 Tick
    Bar_1min = 1011   # 1 分钟 K 线
    Bar_5min = 1012   # 5 分钟 K 线
    Bar_15min = 1013  # 15 分钟 K 线
    Bar_1h = 1014     # 1 小时 K 线
    Bar_1d = 1015     # 日 K 线
    Bar_1w = 1016     # 周 K 线
    Bar_1m = 1017     # 月 K 线
    Trade = 1060      # 成交
    Bid = 1061        # 买价
    Ask = 1062        # 卖价
    Full = 1063       # 全档
    BidPrice = 1064   # 买一价
    BidSize = 1065    # 买一量
    AskPrice = 1066   # 卖一价
    AskSize = 1067    # 卖一量
    Price = 1068      # 价格
    TradeSize = 1069  # 成交量
    OpenPrice = 1070  # 开盘价
    HighPrice = 1071  # 最高价
    LowPrice = 1072   # 最低价
    ClosePrice = 1073 # 收盘价
    Volume = 1074     # 成交量
    OpenInterest = 1075  # 持仓量
    Bar = 1076        # K 线

# =============================================================================
# 交易方向、开平、期权类型等（与 vnpy 风格兼容）
# =============================================================================


class Direction(Enum):
    """
    交易方向：订单/成交/持仓的买卖方向。
    """
    LONG = "多"   # 多头/买入
    SHORT = "空"  # 空头/卖出
    NET = "净"    # 净头寸（期权等）


# 方向映射：StarQuant/VnPy -> CTP
DIRECTION_VT2CTP = {
    Direction.LONG: THOST_FTDC_D_Buy,
    Direction.SHORT: THOST_FTDC_D_Sell,
    Direction.NET: THOST_FTDC_PD_Net
}
# 方向映射：CTP -> StarQuant/VnPy（含持仓方向 Long/Short）
DIRECTION_CTP2VT = {v: k for k, v in DIRECTION_VT2CTP.items()}
DIRECTION_CTP2VT[THOST_FTDC_PD_Long] = Direction.LONG
DIRECTION_CTP2VT[THOST_FTDC_PD_Short] = Direction.SHORT


class Offset(Enum):
    """
    开平标志：订单/成交的开仓或平仓类型。
    """
    NONE = ""           # 无/不区分
    OPEN = "开"         # 开仓
    CLOSE = "平"        # 平仓
    CLOSETODAY = "平今"  # 平今
    CLOSEYESTERDAY = "平昨"  # 平昨


# OrderFlag -> Offset 映射
ORDERFALG_2VT = {
    OrderFlag.OPEN: Offset.OPEN,
    OrderFlag.CLOSE: Offset.CLOSE,
    OrderFlag.CLOSE_TODAY: Offset.CLOSETODAY,
    OrderFlag.CLOSE_YESTERDAY: Offset.CLOSEYESTERDAY,
}
# Offset -> OrderFlag 反向映射
OFFSET_VT2ORDERFLAG = {v: k for k, v in ORDERFALG_2VT.items()}


class OptionType(Enum):
    """
    期权类型：看涨/看跌。
    """
    CALL = "看涨期权"
    PUT = "看跌期权"


# CTP 期权类型代码 -> OptionType 映射（'1'=看涨，'2'=看跌）
OPTIONTYPE_CTP2VT = {
    '1': OptionType.CALL,
    '2': OptionType.PUT
}


class Status(Enum):
    """
    订单状态（VnPy 风格，用于 UI 展示）。

    与 OrderStatus 对应，提供中文描述。
    """
    NEWBORN = "等待提交"
    SUBMITTING = "提交中"
    NOTTRADED = "未成交"
    PARTTRADED = "部分成交"
    ALLTRADED = "全部成交"
    CANCELLED = "已撤销"
    REJECTED = "拒单"
    UNKNOWN = "未知"


# OrderStatus -> Status 映射，用于统一展示
ORDERSTATUS_2VT = {
    OrderStatus.SUBMITTED: Status.SUBMITTING,
    OrderStatus.NEWBORN: Status.NEWBORN,
    OrderStatus.UNKNOWN: Status.UNKNOWN,
    OrderStatus.ACKNOWLEDGED: Status.NOTTRADED,
    OrderStatus.PARTIALLY_FILLED: Status.PARTTRADED,
    OrderStatus.FILLED: Status.ALLTRADED,
    OrderStatus.CANCELED: Status.CANCELLED,
    OrderStatus.ERROR: Status.REJECTED
}

# 仍处于活跃状态的订单状态集合（未完成/未撤销/未拒单）
ACTIVE_STATUSES = set([Status.NEWBORN, Status.SUBMITTING,
                       Status.NOTTRADED, Status.PARTTRADED, Status.UNKNOWN])


class Product(Enum):
    """
    产品类型：标的资产类别。
    """
    EQUITY = "股票"
    FUTURES = "期货"
    OPTION = "期权"
    INDEX = "指数"
    FOREX = "外汇"
    SPOT = "现货"
    ETF = "ETF"
    BOND = "债券"
    WARRANT = "权证"
    SPREAD = "价差"
    FUND = "基金"


# CTP 产品类型代码 -> Product 映射
PRODUCT_CTP2VT = {
    '1': Product.FUTURES,
    '2': Product.OPTION,
    '3': Product.SPREAD,
    '5': Product.SPOT
}

# Product -> StarQuant 产品代码映射（用于 full_symbol 等）
PRODUCT_VT2SQ = {
    Product.EQUITY: "T",
    Product.FUTURES: "F",
    Product.OPTION: "O",
    Product.INDEX: "Z",
    Product.FOREX: "X",
    Product.SPOT: "P",
    Product.ETF: "e",
    Product.BOND: "B",
    Product.WARRANT: "W",
    Product.SPREAD: "S",
    Product.FUND: "J"
}


class Exchange(Enum):
    """
    交易所枚举。

    支持国内期货、股票及部分境外/加密货币交易所。
    """
    # ----- 国内 -----
    SHFE = "SHFE"   # 上期所
    CZCE = "CZCE"   # 郑商所
    DCE = "DCE"     # 大商所
    CFFEX = "CFFEX" # 中金所
    INE = "INE"     # 能源中心
    SSE = "SSE"     # 上交所
    SZSE = "SZSE"   # 深交所
    SGE = "SGE"     # 上金所

    # ----- 境外 -----
    SMART = "SMART"     # IB 智能路由
    NYMEX = "NYMEX"     # 纽约商业交易所
    GLOBEX = "GLOBEX"   # CME 电子盘
    IDEALPRO = "IDEALPRO"  # IB 外汇
    CME = "CME"         # 芝加哥商业交易所
    ICE = "ICE"         # 洲际交易所
    SEHK = "SEHK"       # 港交所
    HKFE = "HKFE"       # 港期所

    # ----- 加密货币 -----
    BITMEX = "BITMEX"
    OKEX = "OKEX"
    HUOBI = "HUOBI"
    BITFINEX = "BITFINEX"


# CTP 交易所代码 -> Exchange 映射（国内期货）
EXCHANGE_CTP2VT = {
    "CFFEX": Exchange.CFFEX,
    "SHFE": Exchange.SHFE,
    "CZCE": Exchange.CZCE,
    "DCE": Exchange.DCE,
    "INE": Exchange.INE
}


class Currency(Enum):
    """
    货币类型。
    """
    USD = "USD"   # 美元
    HKD = "HKD"   # 港币
    CNY = "CNY"   # 人民币


class Interval(Enum):
    """
    K 线周期：Bar 数据的时间间隔。
    """
    MINUTE = "1m"   # 1 分钟
    HOUR = "1h"     # 1 小时
    DAILY = "d"     # 日线
    WEEKLY = "w"    # 周线


class EngineType(Enum):
    """
    引擎运行模式：实盘或回测。
    """
    LIVE = "实盘"       # 实盘交易
    BACKTESTING = "回测"  # 历史回测


class BacktestingMode(Enum):
    """
    回测模式：按 K 线驱动或按 Tick 驱动。
    """
    BAR = 1   # K 线模式，按 Bar 推进
    TICK = 2  # Tick 模式，按逐笔推进


class StopOrderStatus(Enum):
    """
    条件单/止损单状态。
    """
    WAITING = "等待中"   # 等待触发
    CANCELLED = "已撤销"  # 已撤销
    TRIGGERED = "已触发"  # 已触发


# 条件单 ID 前缀，用于区分普通订单
STOPORDER_PREFIX = "STOP"
