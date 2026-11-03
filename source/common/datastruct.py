#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 数据结构模块

本模块定义事件驱动架构中的核心数据结构，包括：
- Event：事件基类，用于组件间消息传递
- 行情数据：TickData、BarData
- 交易数据：OrderData、TradeData、PositionData、AccountData
- 合约与请求：ContractData、SubscribeRequest、CancelRequest 等
- 序列化/反序列化：支持管道符分隔的字符串格式，用于进程间通信
"""

from pandas import Timestamp
import pandas as pd
from dataclasses import dataclass, field
from datetime import datetime
from logging import INFO
from typing import Any, Callable

from .constant import (EventType, MSG_TYPE, Exchange, Interval,
                       OrderType, Direction, Offset, Status, OrderFlag, OrderStatus, ACTIVE_STATUSES, StopOrderStatus,
                       DIRECTION_CTP2VT, ORDERFALG_2VT, ORDERSTATUS_2VT,
                       Product, OptionType, SYMBOL_TYPE,
                       OPTIONTYPE_CTP2VT, PRODUCT_CTP2VT, PRODUCT_VT2SQ, EXCHANGE_CTP2VT)
from ..api.ctp_constant import THOST_FTDC_PT_Net
from .utility import generate_full_symbol, extract_full_symbol, generate_vt_symbol, virtual


def retrieve_multiplier_from_full_symbol(symbol=""):
    """
    根据 full_symbol 获取合约乘数。

    当前为占位实现，始终返回 1.0。实际应从合约信息中查询。
    """
    return 1.0


# =============================================================================
# 事件与数据基类
# =============================================================================


class Event(object):
    """
    事件基类，用于事件驱动架构中的消息传递。

    每个事件包含类型、负载数据、目标/来源组件及消息类型，
    支持序列化为管道符分隔的字符串，用于进程间通信。
    """

    def __init__(self,
                 type: EventType = EventType.HEADER,
                 data: Any = None,
                 des: str = '',
                 src: str = '',
                 msgtype: MSG_TYPE = MSG_TYPE.MSG_TYPE_BASE
                 ):
        self.event_type = type   # 事件类型
        self.data = data         # 负载数据（TickData、OrderData 等）
        self.destination = des   # 目标组件
        self.source = src        # 来源组件
        self.msg_type = msgtype  # 消息类型枚举

    @property
    def type(self):
        """事件类型名称（字符串）"""
        return self.event_type.name

    def serialize(self):
        """序列化为 '目标|来源|消息类型|数据' 格式的字符串"""
        msg = self.destination + '|' + self.source + \
            '|' + str(self.msg_type.value)
        if self.data:
            if type(self.data) == str:
                msg = msg + '|' + self.data
            else:
                try:
                    msg = msg + '|' + self.data.serialize()
                except Exception as e:
                    print(e)
                    pass
        return msg

    def deserialize(self, msg: str):
        """
        从序列化字符串反序列化事件。

        根据消息类型自动创建对应的 data 对象（TickData、OrderData 等）
        并调用其 deserialize 解析第四段及之后的内容。
        """
        v = msg.split('|', 3)
        try:
            self.destination = v[0]
            self.source = v[1]
            msg2type = MSG_TYPE(int(v[2]))
            if msg2type in [MSG_TYPE.MSG_TYPE_TICK, MSG_TYPE.MSG_TYPE_TICK_L1, MSG_TYPE.MSG_TYPE_TICK_L5]:
                self.event_type = EventType.TICK
                self.data = TickData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif msg2type == MSG_TYPE.MSG_TYPE_RTN_ORDER:
                self.event_type = EventType.ORDERSTATUS
                self.data = OrderData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif msg2type == MSG_TYPE.MSG_TYPE_RTN_TRADE:
                self.event_type = EventType.FILL
                self.data = TradeData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif msg2type == MSG_TYPE.MSG_TYPE_RSP_POS:
                self.event_type = EventType.POSITION
                self.data = PositionData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif msg2type == MSG_TYPE.MSG_TYPE_BAR:
                self.event_type = EventType.BAR
                self.data = BarData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif msg2type == MSG_TYPE.MSG_TYPE_RSP_ACCOUNT:
                self.event_type = EventType.ACCOUNT
                self.data = AccountData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif msg2type == MSG_TYPE.MSG_TYPE_RSP_CONTRACT:
                self.event_type = EventType.CONTRACT
                self.data = ContractData(gateway_name=self.source)
                self.data.deserialize(v[3])
            elif v[2].startswith('11'):
                self.event_type = EventType.ENGINE_CONTROL
                self.msg_type = msg2type
                if len(v) > 3:
                    self.data = v[3]
            elif v[2].startswith('12'):
                self.event_type = EventType.STRATEGY_CONTROL
                self.msg_type = msg2type
                if len(v) > 3:
                    self.data = v[3]
            elif v[2].startswith('14'):
                self.event_type = EventType.RECORDER_CONTROL
                self.msg_type = msg2type
                if len(v) > 3:
                    self.data = v[3]
            elif v[2].startswith('3'):  # msg2type == MSG_TYPE.MSG_TYPE_INFO:
                self.event_type = EventType.INFO
                self.msg_type = msg2type
                self.data = LogData(gateway_name=self.source)
                self.data.deserialize(v[3])
            else:
                self.event_type = EventType.HEADER
                self.msg_type = msg2type
        except Exception as e:
            print(e)
            pass


@dataclass
class BaseData:
    """
    数据对象基类。

    所有行情、订单、成交等数据均需继承此类，并包含 gateway_name 标识数据来源。
    子类需实现 serialize/deserialize 以支持进程间传递。
    """

    gateway_name: str = ''  # 数据来源网关/引擎名称

    @virtual
    def serialize(self):
        """序列化为字符串，子类实现"""
        pass

    def deserialize(self, msg):
        """从字符串反序列化，子类实现"""
        pass


@dataclass
class TickData(BaseData):
    """
    逐笔行情数据。

    包含最新成交、买卖盘口快照（1~5 档）、当日统计等。
    支持 1 档与 5 档两种序列化格式。
    """

    symbol: str = ""
    exchange: Exchange = Exchange.SHFE
    datetime: datetime = datetime(2019, 1, 1)

    name: str = ""
    volume: float = 0          # 当日成交量
    last_price: float = 0      # 最新价
    last_volume: float = 0    # 最新成交量
    limit_up: float = 0       # 涨停价
    limit_down: float = 0     # 跌停价

    open_price: float = 0     # 开盘价
    high_price: float = 0     # 最高价
    low_price: float = 0      # 最低价
    pre_close: float = 0      # 昨收价

    bid_price_1: float = 0    # 买一价
    bid_price_2: float = 0    # 买二价
    bid_price_3: float = 0
    bid_price_4: float = 0
    bid_price_5: float = 0

    ask_price_1: float = 0    # 卖一价
    ask_price_2: float = 0
    ask_price_3: float = 0
    ask_price_4: float = 0
    ask_price_5: float = 0

    bid_volume_1: float = 0   # 买一量
    bid_volume_2: float = 0
    bid_volume_3: float = 0
    bid_volume_4: float = 0
    bid_volume_5: float = 0

    ask_volume_1: float = 0   # 卖一量
    ask_volume_2: float = 0
    ask_volume_3: float = 0
    ask_volume_4: float = 0
    ask_volume_5: float = 0

    depth: int = 0            # 档位深度（1 或 5）
    open_interest: float = 0   # 持仓量（期货）

    def __post_init__(self):
        """初始化时生成 vt_symbol、timestamp、full_symbol"""
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
        self.timestamp = Timestamp(self.datetime)
        self.full_symbol = generate_full_symbol(self.exchange, self.symbol)

    def deserialize(self, msg: str):
        """
        从管道符分隔字符串反序列化。
        格式：full_symbol|datetime|last_price|volume|...（1 档 17 段，5 档 31 段）
        """
        try:
            v = msg.split('|')
            self.full_symbol = v[0]
            self.timestamp = pd.to_datetime(v[1])
            self.datetime = self.timestamp.to_pydatetime()
            self.symbol, self.exchange = extract_full_symbol(self.full_symbol)
            self.vt_symbol = generate_vt_symbol(self.symbol, self.exchange)
            self.last_price = float(v[2])
            self.volume = int(v[3])

            if (len(v) < 17):
                self.depth = 1
                self.bid_price_1 = float(v[4])
                self.bid_volume_1 = int(v[5])
                self.ask_price_1 = float(v[6])
                self.ask_volume_1 = int(v[7])
                self.open_interest = int(v[8])
                self.open_price = float(v[9])
                self.high_price = float(v[10])
                self.low_price = float(v[11])
                self.pre_close = float(v[12])
                self.limit_up = float(v[13])
                self.limit_down = float(v[14])
            else:
                self.depth = 5
                self.bid_price_1 = float(v[4])
                self.bid_volume_1 = int(v[5])
                self.ask_price_1 = float(v[6])
                self.ask_volume_1 = int(v[7])
                self.bid_price_2 = float(v[8])
                self.bid_volume_2 = int(v[9])
                self.ask_price_2 = float(v[10])
                self.ask_volume_2 = int(v[11])
                self.bid_price_3 = float(v[12])
                self.bid_volume_3 = int(v[13])
                self.ask_price_3 = float(v[14])
                self.ask_volume_3 = int(v[15])
                self.bid_price_4 = float(v[16])
                self.bid_volume_4 = int(v[17])
                self.ask_price_4 = float(v[18])
                self.ask_volume_4 = int(v[19])
                self.bid_price_5 = float(v[20])
                self.bid_volume_5 = int(v[21])
                self.ask_price_5 = float(v[22])
                self.ask_volume_5 = int(v[23])
                self.open_interest = int(v[24])
                self.open_price = float(v[25])
                self.high_price = float(v[26])
                self.low_price = float(v[27])
                self.pre_close = float(v[28])
                self.limit_up = float(v[29])
                self.limit_down = float(v[30])
        except Exception as e:
            print(e)
            pass


@dataclass
class BarData(BaseData):
    """
    K 线数据。

    表示某一时间周期内的 OHLCV 及持仓量（期货）。
    """

    symbol: str = ''
    exchange: Exchange = Exchange.SHFE
    datetime: datetime = datetime(2019, 1, 1)

    interval: Interval = None   # K 线周期
    volume: float = 0          # 成交量
    open_price: float = 0     # 开盘价
    high_price: float = 0      # 最高价
    low_price: float = 0       # 最低价
    close_price: float = 0     # 收盘价

    adj_close_price: float = 0.0  # 复权收盘价
    open_interest: int = 0        # 持仓量（期货）

    def __post_init__(self):
        """初始化时生成 vt_symbol、full_symbol、bar_start_time"""
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
        self.full_symbol = generate_full_symbol(self.exchange, self.symbol)
        self.bar_start_time = pd.Timestamp(self.datetime)


@dataclass
class OrderData(BaseData):
    """
    委托数据。

    用于跟踪单笔委托的完整状态，包含价格、数量、成交、状态及各类 ID。
    """

    symbol: str = ""
    exchange: Exchange = Exchange.SHFE
    orderid: str = ""

    type: OrderType = OrderType.LMT      # 订单类型
    direction: Direction = Direction.LONG # 方向
    offset: Offset = Offset.NONE         # 开平
    price: float = 0                    # 委托价
    volume: int = 0                     # 委托量
    traded: int = 0                     # 已成交量
    status: Status = Status.SUBMITTING   # 状态（UI 用）
    time: str = ""                      # 更新时间

    api: str = ""                       # 接口名
    account: str = ""                   # 账户
    clientID: int = -1                  # 客户端 ID
    client_order_id: int = -1           # 本地委托 ID
    tag: str = ""                       # 标签

    full_symbol: str = ""
    flag: OrderFlag = OrderFlag.OPEN    # 开平标志
    server_order_id: int = -1            # 服务器委托 ID
    broker_order_id: int = -1           # 经纪商委托 ID
    orderNo: str = ""                   # 交易所报单号
    localNo: str = ""                   # 本地单号
    create_time: str = ""
    update_time: str = ""
    orders_status: OrderStatus = OrderStatus.SUBMITTED  # 订单状态
    orderfield: Any = None              # 原始报单字段（CTP/Paper）

    def __post_init__(self):
        """根据 full_symbol 解析 symbol/exchange，生成 vt_symbol、vt_orderid"""
        if self.full_symbol:
            self.symbol, self.exchange = extract_full_symbol(self.full_symbol)
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
        self.vt_orderid = f"{self.gateway_name}.{self.orderid}"
        self.tag = str(self.type.value)

    def is_active(self):
        """判断委托是否仍处于活跃状态（未完成/未撤销/未拒单）"""
        if self.status in ACTIVE_STATUSES:
            return True
        else:
            return False

    def create_cancel_request(self):
        """根据当前委托创建撤单请求对象"""
        req = CancelRequest(
            clientID=self.clientID,
            client_order_id=self.client_order_id,
            server_order_id=self.server_order_id
        )
        return req

    def deserialize(self, msg: str):
        """从管道符分隔字符串反序列化委托数据"""
        v = msg.split('|')
        try:
            self.api = v[0]
            self.account = v[1]
            self.clientID = int(v[2])
            self.client_order_id = int(v[3])
            self.tag = v[4]
            self.type = OrderType(int(v[4].split(':')[0]))
            self.full_symbol = v[5]
            self.symbol, self.exchange = extract_full_symbol(self.full_symbol)
            self.vt_symbol = generate_vt_symbol(self.symbol, self.exchange)
            self.price = float(v[6])
            self.volume = int(v[7])
            if self.volume < 0:
                self.direction = Direction.SHORT
                self.volume = -1 * self.volume
            self.traded = abs(int(v[8]))
            self.flag = OrderFlag(int(v[9]))
            self.offset = ORDERFALG_2VT[self.flag]
            self.server_order_id = int(v[10])
            self.broker_order_id = int(v[11])
            self.orderNo = v[12]
            self.localNo = v[13]
            self.orderid = self.localNo
            self.vt_orderid = f"{self.gateway_name}.{self.orderid}"
            self.create_time = v[14]
            self.update_time = v[15]
            self.time = self.update_time
            self.order_status = OrderStatus(int(v[16]))
            self.status = ORDERSTATUS_2VT[self.order_status]
        except Exception as e:
            print(e)
            pass

    def serialize(self):
        """序列化为 'api|account|clientID|client_order_id|tag|orderfield' 格式"""
        msg = str(self.api
                  + '|' + self.account
                  + '|' + str(self.clientID)
                  + '|' + str(self.client_order_id)
                  + '|' + self.tag)
        if (self.orderfield):
            msg = msg + '|' + self.orderfield.serialize()
        return msg


@dataclass
class CtpOrderField(object):
    """
    CTP 报单字段。

    与 CTP API 的 CThostFtdcInputOrderField 对应，
    用于序列化后通过消息队列发送给 CTP 交易接口。
    """

    InstrumentID: str = ''       # 合约代码
    OrderPriceType: str = ''      # 报单价格类型
    Direction: str = ''           # 买卖方向
    CombOffsetFlag: str = ''     # 开平标志
    CombHedgeFlag: str = ''      # 投机套保
    LimitPrice: float = 0.0      # 限价
    VolumeTotalOriginal: int = 0 # 委托数量
    TimeCondition: str = ''      # 有效期类型
    GTDDate: str = ''            # GTD 日期
    VolumeCondition: str = ''     # 成交量类型
    MinVolume: int = 0           # 最小成交量
    ContingentCondition: str = '' # 触发条件
    StopPrice: float = 0.0        # 止损价
    ForceCloseReason: str = '0'   # 强平原因
    IsAutoSuspend: int = 0       # 自动挂起
    UserForceClose: int = 0      # 用户强平
    IsSwapOrder: int = 0         # 互换单
    BusinessUnit: str = ''       # 业务单元
    CurrencyID: str = ''         # 币种

    def serialize(self):
        """序列化为管道符分隔字符串，用于消息传递"""
        msg = str(self.InstrumentID
                  + '|' + self.OrderPriceType
                  + '|' + self.Direction
                  + '|' + self.CombOffsetFlag
                  + '|' + self.CombHedgeFlag
                  + '|' + str(self.LimitPrice)
                  + '|' + str(self.VolumeTotalOriginal)
                  + '|' + self.TimeCondition
                  + '|' + self.GTDDate
                  + '|' + self.VolumeCondition
                  + '|' + str(self.MinVolume)
                  + '|' + self.ContingentCondition
                  + '|' + str(self.StopPrice)
                  + '|' + self.ForceCloseReason
                  + '|' + str(self.IsAutoSuspend)
                  + '|' + str(self.UserForceClose)
                  + '|' + str(self.IsSwapOrder)
                  + '|' + self.BusinessUnit
                  + '|' + self.CurrencyID)
        return msg


@dataclass
class PaperOrderField(object):
    """
    模拟盘报单字段。

    用于 PaperTDEngine 的报单序列化，比 CTP 字段简化。
    """

    order_type: OrderType = OrderType.MKT
    full_symbol: str = ''
    order_flag: OrderFlag = OrderFlag.OPEN
    limit_price: float = 0.0
    stop_price: float = 0.0
    order_size: int = 0

    def serialize(self):
        """序列化为 'order_type|full_symbol|order_flag|order_size|limit_price|stop_price'"""
        msg = str(str(self.order_type.value)
                  + '|' + self.full_symbol
                  + '|' + str(self.order_flag.value)
                  + '|' + str(self.order_size)
                  + '|' + str(self.limit_price)
                  + '|' + str(self.stop_price))
        return msg


@dataclass
class TradeData(BaseData):
    """
    成交数据。

    一笔委托可对应多笔成交，每笔成交记录价格、数量、手续费等。
    含回测用字段（滑点、 turnover、持仓与盈亏）。
    """

    symbol: str = ""
    exchange: Exchange = Exchange.SHFE
    orderid: str = ""
    tradeid: str = ""
    direction: Direction = ""

    offset: Offset = Offset.NONE
    price: float = 0        # 成交价
    volume: float = 0       # 成交量
    time: str = ""

    server_order_id: int = -1
    client_order_id: int = -1
    clientID: int = -1
    localNo: str = ""
    orderNo: str = ""
    full_symbol: str = ""
    fill_flag: OrderFlag = OrderFlag.OPEN
    commission: float = 0.0  # 手续费
    account: str = ""
    api: str = ""

    datetime: datetime = datetime(1990, 1, 1)

    slippage: float = 0.0   # 滑点（回测）
    turnover: float = 0.0    # 成交额（回测）
    long_pos: int = 0       # 多头持仓（回测）
    long_price: float = 0
    long_pnl: float = 0
    short_pos: int = 0      # 空头持仓（回测）
    short_price: float = 0
    short_pnl: float = 0

    def __post_init__(self):
        """初始化时生成 vt_symbol、vt_orderid、vt_tradeid"""
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
        self.vt_orderid = f"{self.gateway_name}.{self.orderid}"
        self.vt_tradeid = f"{self.gateway_name}.{self.tradeid}"

    def deserialize(self, msg: str):
        """从管道符分隔字符串反序列化成交数据"""
        v = msg.split('|')
        try:
            self.server_order_id = int(v[0])
            self.client_order_id = int(v[1])
            self.clientID = int(v[2])
            self.localNo = v[3]
            self.orderid = self.localNo
            self.vt_orderid = f"{self.gateway_name}.{self.orderid}"
            self.orderNo = v[4]
            self.tradeid = v[5]
            self.vt_tradeid = f"{self.gateway_name}.{self.tradeid}"
            self.time = v[6]
            self.full_symbol = v[7]
            self.symbol, self.exchange = extract_full_symbol(self.full_symbol)
            self.vt_symbol = f"{self.symbol}.{self.exchange.value}"

            self.price = float(v[8])
            quantity = int(v[9])
            self.volume = abs(quantity)
            self.direction = Direction.LONG if quantity > 0 else Direction.SHORT
            self.fill_flag = OrderFlag(int(v[10]))
            self.offset = ORDERFALG_2VT[self.fill_flag]

            self.commission = float(v[11])
            self.account = v[12]
            self.api = v[13]
        except Exception as e:
            print(e)
            pass


@dataclass
class PositionData(BaseData):
    """
    持仓数据。

    跟踪单边持仓（多/空），包含数量、冻结、均价、盈亏等。
    """

    symbol: str = ""
    exchange: Exchange = Exchange.SHFE
    direction: Direction = Direction.LONG

    volume: float = 0       # 持仓量
    frozen: float = 0        # 冻结量
    price: float = 0         # 持仓均价
    pnl: float = 0           # 浮动盈亏
    yd_volume: float = 0     # 昨仓量

    key: str = ""           # 持仓唯一键
    account: str = ""
    api: str = ""
    full_symbol: str = ""
    realized_pnl: float = 0  # 已实现盈亏
    timestamp: str = ""

    def __post_init__(self):
        """初始化时生成 vt_symbol、vt_positionid"""
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
        self.vt_positionid = f"{self.vt_symbol}.{self.direction}"

    def deserialize(self, msg: str):
        """从管道符分隔字符串反序列化持仓数据"""
        v = msg.split('|')
        try:
            self.key = v[0]
            self.account = v[1]
            self.api = v[2]
            self.full_symbol = v[3]
            self.symbol, self.exchange = extract_full_symbol(self.full_symbol)
            self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
            self.direction = DIRECTION_CTP2VT[v[4]]
            self.price = float(v[5])
            self.vt_positionid = f"{self.vt_symbol}.{self.direction}"
            self.volume = abs(int(v[6]))
            self.yd_volume = abs(int(v[7]))
            self.freezed_size = abs(int(v[8]))
            self.realized_pnl = float(v[9])
            self.pnl = float(v[10])
            self.timestamp = v[11]
        except Exception as e:
            print(e)
            pass


# 回测成交数据别名（与 TradeData 相同）
BacktestTradeData = TradeData


@dataclass
class AccountData(BaseData):
    """
    账户数据。

    包含资金余额、冻结、可用及盈亏统计。
    """

    accountid: str = ""

    balance: float = 0      # 总权益
    frozen: float = 0       # 冻结资金

    yd_balance: float = 0   # 昨日权益
    netliquid: float = 0    # 净值
    commission: float = 0    # 手续费
    margin: float = 0       # 保证金
    closed_pnl: float = 0   # 平仓盈亏
    open_pnl: float = 0     # 持仓盈亏
    timestamp: str = ""

    def __post_init__(self):
        """初始化时计算 available、vt_accountid"""
        self.available = self.balance - self.frozen
        self.vt_accountid = f"{self.gateway_name}.{self.accountid}"

    def deserialize(self, msg: str):
        """从管道符分隔字符串反序列化账户数据"""
        v = msg.split('|')
        try:
            self.accountid = v[0]
            self.vt_accountid = f"{self.gateway_name}.{self.accountid}"
            self.yd_balance = float(v[1])
            self.netliquid = float(v[2])
            self.available = float(v[3])
            self.commission = float(v[4])
            self.margin = float(v[5])
            self.closed_pnl = float(v[6])
            self.open_pnl = float(v[7])
            self.balance = float(v[8])
            self.frozen = float(v[9])
            self.timestamp = v[10]
        except Exception as e:
            print(e)
            pass


@dataclass
class LogData(BaseData):
    """
    日志数据。

    用于在 GUI 或日志文件中记录日志消息。
    """

    msg: str = ''
    level: int = INFO   # 日志级别
    timestamp: str = ''

    def __post_init__(self):
        """初始化时设置 time 为当前时间"""
        self.time = datetime.now()

    def deserialize(self, msg: str):
        """从管道符分隔字符串反序列化日志数据"""
        v = msg.split('|')
        try:
            self.msg = v[0]
            self.timestamp = v[1]
        except Exception as e:
            print(e)
            pass


@dataclass
class ContractData(BaseData):
    """
    合约数据。

    包含合约基本信息：乘数、最小变动、保证金、期权参数等。
    """

    symbol: str = ""
    exchange: Exchange = Exchange.SHFE
    name: str = ""
    product: Product = Product.FUTURES
    size: int = 1              # 合约乘数
    pricetick: float = 0       # 最小变动价位

    min_volume: float = 1      # 最小交易手数
    stop_supported: bool = False  # 是否支持止损单
    net_position: bool = False    # 是否净持仓模式

    option_strike: float = 0     # 期权行权价
    option_underlying: str = ""  # 标的合约 vt_symbol
    option_type: OptionType = None
    option_expiry: datetime = None

    full_symbol: str = ""
    long_margin_ratio: float = 0   # 多头保证金比例
    short_margin_ratio: float = 0  # 空头保证金比例

    def __post_init__(self):
        """初始化时生成 vt_symbol"""
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"

    def deserialize(self, msg: str):
        """从管道符分隔字符串反序列化合约数据"""
        v = msg.split('|')
        try:
            self.symbol = v[0]
            self.exchange = EXCHANGE_CTP2VT[v[1]]
            self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
            self.name = v[2]
            self.product = PRODUCT_CTP2VT.get(v[3], None)
            st = PRODUCT_VT2SQ[self.product]
            self.full_symbol = generate_full_symbol(
                self.exchange, self.symbol, st)
            self.size = int(v[4])
            self.pricetick = float(v[5])
            if v[6] == THOST_FTDC_PT_Net:
                self.net_position = True
            self.long_margin_ratio = float(v[7])
            self.short_margin_ratio = float(v[8])
            if self.product == Product.OPTION:
                self.option_underlying = v[9]
                self.option_type = OPTIONTYPE_CTP2VT.get(v[10], None)
                self.option_strike = float(v[11])
                self.option_expiry = datetime.strptime(v[12], "%Y%m%d")
        except Exception as e:
            print(e)
            pass

# product = PRODUCT_CTP2VT.get(data["ProductClass"], None)
# if product:
# OPTIONTYPE_CTP2VT.get(data["OptionsType"], None),


@dataclass
class StopOrder:
    """
    条件单/止损单。

    记录触发价格、数量、关联的委托 ID 及状态。
    """

    full_symbol: str
    direction: Direction
    offset: Offset
    price: float          # 触发价
    volume: float
    stop_orderid: str     # 条件单 ID
    strategy_name: str
    lock: bool = False    # 是否锁定
    vt_orderids: list = field(default_factory=list)  # 触发后产生的委托 ID
    status: StopOrderStatus = StopOrderStatus.WAITING


@dataclass
class QryContractRequest:
    """
    合约查询请求。

    用于向网关请求合约列表，支持 full_symbol 或 CTP 格式。
    """
    sym_type: SYMBOL_TYPE = SYMBOL_TYPE.FULL
    content: str = ''

    def serialize(self):
        """序列化为 'sym_type|content'"""
        msg = str(self.sym_type.value) + '|' + self.content
        return msg


@dataclass
class SubscribeRequest:
    """
    行情订阅请求。

    用于向行情引擎订阅指定合约的行情。
    """
    sym_type: SYMBOL_TYPE = SYMBOL_TYPE.FULL
    content: str = ''

    def serialize(self):
        """序列化为 'sym_type|content'"""
        msg = str(self.sym_type.value) + '|' + self.content
        return msg


@dataclass
class CancelRequest:
    """
    撤单请求。

    发送给网关用于撤销指定委托，需提供 client_order_id 或 server_order_id。
    """
    clientID: int = 0
    client_order_id: int = 0
    server_order_id: int = 0

    def serialize(self):
        """序列化为 'clientID|client_order_id|server_order_id'"""
        msg = str(self.clientID) + '|' + str(self.client_order_id) + \
            '|' + str(self.server_order_id)
        return msg


# 下单请求使用 OrderData
OrderRequest = OrderData
# 全部撤单请求复用 SubscribeRequest 格式
CancelAllRequest = SubscribeRequest


@dataclass
class HistoryRequest:
    """
    历史数据查询请求。

    用于向数据源请求指定时间范围、周期的 K 线或 Tick 数据。
    """

    symbol: str
    exchange: Exchange
    start: datetime
    end: datetime = None
    interval: Interval = None

    def __post_init__(self):
        """初始化时生成 vt_symbol"""
        self.vt_symbol = f"{self.symbol}.{self.exchange.value}"
