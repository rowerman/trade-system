"""
StarQuant 米筐数据（RQData）客户端

通过 rqdatac 从米筐获取历史 K 线数据，
支持期货、股票合约的 symbol 转换及时间对齐。
"""
from datetime import datetime, timedelta
from typing import List

# 用于与金融数据平台RQData交互，提供高效的金融数据获取、处理和分析功能
from rqdatac import init as rqdata_init
from rqdatac.services.basic import all_instruments as rqdata_all_instruments
from rqdatac.services.get_price import get_price as rqdata_get_price

from source.common.config import SETTINGS
from source.common.datastruct import BarData, HistoryRequest
from source.common.constant import Exchange, Interval


# StarQuant Interval -> RQData frequency 映射
INTERVAL_VT2RQ = {
    Interval.MINUTE: "1m",
    Interval.HOUR: "60m",
    Interval.DAILY: "1d",
}

# 将 RQData 的收盘时刻调整为 K 线开盘时刻的偏移（VN Trader 风格）
INTERVAL_ADJUSTMENT_MAP = {
    Interval.MINUTE: timedelta(minutes=1),
    Interval.HOUR: timedelta(hours=1),
    Interval.DAILY: timedelta()         # no need to adjust for daily bar
}


class RqdataClient:
    """
    Client for querying history data from RQData.
    """

    def __init__(self):
        """从配置读取 rqdata 账号密码"""
        self.username = SETTINGS["rqdata.username"]
        self.password = SETTINGS["rqdata.password"]

        self.inited = False
        self.symbols = set()

    def init(self):
        """
        初始化 RQData 连接并拉取全市场合约列表。
        成功返回 True，失败返回 False。
        """
        if self.inited:
            return True

        if not self.username or not self.password:
            return False

        rqdata_init(self.username, self.password,
                    ('rqdatad-pro.ricequant.com', 16011))

        try:
            df = rqdata_all_instruments(date=datetime.now())
            for ix, row in df.iterrows():
                self.symbols.add(row['order_book_id'])
        except RuntimeError:
            return False

        self.inited = True
        return True

    def to_rq_symbol(self, symbol: str, exchange: Exchange):
        """
        将 StarQuant 合约代码转为 RQData 格式。

        郑商所：RQData 为 "TA1905"，vt 为 "TA905"，需补 "1" 到年份前。
        股票：上交所 .XSHG，深交所 .XSHE。
        """
        if exchange in [Exchange.SSE, Exchange.SZSE]:
            if exchange == Exchange.SSE:
                rq_symbol = f"{symbol}.XSHG"
            else:
                rq_symbol = f"{symbol}.XSHE"
        else:
            if exchange is not Exchange.CZCE:
                return symbol.upper()

            for count, word in enumerate(symbol):
                if word.isdigit():
                    break

            # noinspection PyUnboundLocalVariable
            product = symbol[:count]
            year = symbol[count]
            month = symbol[count + 1:]

            if year == "9":
                year = "1" + year
            else:
                year = "2" + year

            rq_symbol = f"{product}{year}{month}".upper()

        return rq_symbol

    def query_bar(
        self,
        symbol: str,
        exchange: Exchange,
        interval: Interval,
        start: datetime,
        end: datetime
    ):
        """
        Query bar data from RQData.
        """
        rq_symbol = self.to_rq_symbol(symbol, exchange)
        if rq_symbol not in self.symbols:
            return None

        end += timedelta(1)     # 多查一天以包含夜盘

        df = rqdata_get_price(
            rq_symbol,
            frequency=interval.value,
            fields=["open", "high", "low", "close", "volume","open_interest"],
            start_date=start,
            end_date=end
        )

        data: List[BarData] = []
        for ix, row in df.iterrows():
            bar = BarData(
                symbol=symbol,
                exchange=exchange,
                interval=interval,
                datetime=row.name.to_pydatetime(),
                open_price=row["open"],
                high_price=row["high"],
                low_price=row["low"],
                close_price=row["close"],
                volume=row["volume"],
                open_interest=row["open_interest"],
                gateway_name="RQ"
            )
            data.append(bar)

        return data

    def query_history(self, req: HistoryRequest):
        """
        Query history bar data from RQData.
        """
        symbol = req.symbol
        exchange = req.exchange
        interval = req.interval
        start = req.start
        end = req.end

        rq_symbol = self.to_rq_symbol(symbol, exchange)
        if rq_symbol not in self.symbols:
            return None
        rq_interval = INTERVAL_VT2RQ.get(interval)
        if not rq_interval:
            return None

        # 将 RQData 的收盘时刻调整为 K 线开盘时刻
        adjustment = INTERVAL_ADJUSTMENT_MAP[interval]
        end += timedelta(1)     # 多查一天以包含夜盘

        df = rqdata_get_price(
            rq_symbol,
            frequency=rq_interval,
            fields=["open", "high", "low", "close", "volume","open_interest"],
            start_date=start,
            end_date=end
        )

        data: List[BarData] = []
        for ix, row in df.iterrows():
            bar = BarData(
                symbol=symbol,
                exchange=exchange,
                interval=interval,
                datetime=row.name.to_pydatetime() - adjustment,
                open_price=row["open"],
                high_price=row["high"],
                low_price=row["low"],
                close_price=row["close"],
                volume=row["volume"],
                open_interest=row["open_interest"],
                gateway_name="RQ"
            )
            data.append(bar)

        return data


# 全局 RQData 客户端实例
rqdata_client = RqdataClient()
