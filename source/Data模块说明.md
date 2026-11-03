# Data 模块说明

## 1. 模块概述

`source/data/` 负责**历史与实时数据的存储、加载与加工**：抽象数据库接口，支持 SQL（SQLite/MySQL/PostgreSQL）与 MongoDB；从 RQData 拉取历史 K 线；提供由 Tick 合成 K 线的 BarGenerator 以及基于 K 线序列的技术指标 ArrayManager。回测引擎、策略引擎、录播引擎均依赖本模块读写 Bar/Tick。

## 2. 目录结构

```
source/data/
├── __init__.py        # 导出 database_manager、init、Driver 等
├── database.py        # 驱动枚举与抽象接口 BaseDatabaseManager
├── database_sql.py    # SQL 实现（Peewee ORM）
├── database_mongo.py  # MongoDB 实现（mongoengine）
├── data_board.py      # BarGenerator、ArrayManager（K 线合成与指标）
└── rqdata.py         # RQData 客户端（历史 Bar 查询）
```

## 3. 各文件功能与逻辑

### 3.1 `database.py`

- **Driver**：枚举 `SQLITE`、`MYSQL`、`POSTGRESQL`、`MONGODB`，对应配置中的 `database.driver`。
- **BaseDatabaseManager**：抽象基类，定义统一接口：
  - `load_bar_data(symbol, exchange, interval, start, end)` → BarData 序列
  - `load_tick_data(symbol, exchange, start, end)` → TickData 序列
  - `save_bar_data(datas)` / `save_tick_data(datas)`
  - `get_newest_bar_data(symbol, exchange, interval)` / `get_newest_tick_data(symbol, exchange)` → 单条最新数据或 None
  - `clean(symbol)`：按合约删除该 symbol 的 Bar/Tick 记录

所有具体存储实现都必须实现上述方法，保证上层（回测、录播）可切换数据源而不改业务代码。

### 3.2 `database_sql.py`

- **init(driver, settings)**：根据 Driver 调用 `init_sqlite` / `init_mysql` / `init_postgresql`，得到 Peewee 的 Database 实例；再 `init_models(db, driver)` 得到 Bar/Tick 的 Model 类，最后返回 **SqlManager** 实例。
- **DbBarData / DbTickData**：Peewee Model，字段与 BarData/TickData 对应；唯一索引分别为 (datetime, interval, symbol, exchange) 与 (datetime, symbol, exchange)。提供 `from_bar` / `to_bar`、`from_tick` / `to_tick` 与批量 `save_all`（存在则替换）。
- **SqlManager**：实现 BaseDatabaseManager 的全部方法，内部用 Model 的 select/where/order_by 查询，用 `from_*`/`to_*` 与 common.datastruct 的 BarData/TickData 互转；PostgreSQL 使用 on_conflict 更新，其它库使用 `on_conflict_replace`。

SQL 路径下，SQLite 的 database 配置项被当作文件路径（通过 `get_file_path` 解析到 `.StarQuant` 目录）。

### 3.3 `database_mongo.py`

- **init(_driver, settings)**：使用 mongoengine 的 `connect()` 连接 MongoDB（支持用户名、密码、authentication_source），返回 **MongoManager**。
- **DbBarData / DbTickData**：mongoengine Document，字段与 Bar/Tick 一致，meta 中定义 (datetime, interval, symbol, exchange) 与 (datetime, symbol, exchange) 唯一索引。
- **MongoManager**：实现 BaseDatabaseManager；`save_*` 通过 `update_one(upsert=True)` 按唯一键更新；查询用 `objects(...).order_by("-datetime")` 等。

Bar/Tick 的 “gateway_name / vt_symbol / full_symbol” 等派生字段在保存前从 updates 中剔除，避免写入库。

### 3.4 `__init__.py` 与全局 database_manager

- 使用 `get_settings("database.")` 读取配置，根据 `driver` 调用 `init_sql` 或 `init_nosql`，得到 **database_manager**（类型为 BaseDatabaseManager 的某一实现）。
- 回测引擎、录播引擎通过 `from source.data import database_manager` 直接调用 `load_bar_data`、`save_tick_data` 等，无需关心底层是 SQL 还是 MongoDB。

### 3.5 `data_board.py`

- **BarGenerator**  
  - 用途：从 Tick 合成 1 分钟 Bar；从 1 分钟 Bar 合成多分钟或多小时 Bar。  
  - 构造：`on_bar`（1 分钟 Bar 回调）、`on_window_bar`（多分钟/多小时 Bar 回调）、`window`（合成周期数）、`interval`（MINUTE/HOUR）。  
  - `update_tick(tick)`：过滤无效价/量；按分钟切新 Bar；更新 OHLC、量、持仓量；若有上一 tick 则用 volume 差累加成交量；支持跨日切换时 flush 上一 Bar。  
  - `update_bar(bar)`：把 1 分钟 Bar 并入当前 window_bar，更新 high/low/close/volume；根据 interval 与 window 判断是否完成一个 window_bar，完成则调用 `on_window_bar`。  
  - 约束：多分钟 Bar 的周期需能整除 60（如 2、3、5、15）；多小时可为任意整数。

- **ArrayManager**  
  - 用途：维护固定长度的 OHLCV 数组，并基于此计算技术指标。  
  - 构造：`size`（默认 100），内部用 numpy 数组存 open/high/low/close/volume；`update_bar(bar)` 滚动更新最后一根。  
  - 指标方法：`sma`、`std`、`cci`、`atr`、`rsi`、`macd`、`adx`、`boll`、`keltner`、`donchian` 等，均基于 talib；多数支持 `array=True` 返回整列。  
  - **highlow**：自定义逻辑，用于判断高低点与价格条件（PriceA/PriceB 等），与策略逻辑相关。

RecorderEngine 中每个录播合约对应一个 BarGenerator，收到 Tick 后先录 Tick，再对需要录 Bar 的合约调用 `BarGenerator.update_tick`，在 `on_window_bar` 里将 Bar 写入 database_manager。

### 3.6 `rqdata.py`

- **RqdataClient**：使用 rqdatac 连接米筐（`rqdatad-pro.ricequant.com:16011`），配置来自 SETTINGS 的 `rqdata.username` / `rqdata.password`。
- **init()**：若未初始化则调用 `rqdata_init`，并拉取当日 `all_instruments` 填充 `symbols` 集合，用于校验合约是否支持。
- **to_rq_symbol(symbol, exchange)**：将内部 symbol+exchange 转为 RQ 格式（如 CZCE 合约需补年份 “1”/“2”；股票为 symbol.XSHG/XSHE）。
- **query_bar(symbol, exchange, interval, start, end)**：调用 `get_price` 取 open/high/low/close/volume/open_interest，遍历 DataFrame 转成 BarData 列表；end 会加一天以包含夜盘。
- **query_history(req)**：按 HistoryRequest 的 symbol、exchange、interval、start、end 查询，并对 Bar 时间做 `INTERVAL_ADJUSTMENT_MAP` 偏移（RQ 为收盘时刻，内部部分逻辑用开盘时刻）。

策略引擎在需要预加载历史或补充数据时可使用 rqdata_client；回测若数据源选 RQData，也会通过此类接口取数（需在业务层封装调用）。

## 4. 数据流小结

- **写入**：RecorderEngine 收到 Tick → BarGenerator 合成 Bar → database_manager.save_tick_data / save_bar_data；RQData 仅读不写。
- **读取**：BacktestingEngine 根据 datasource 选 DataBase 或 Memory；DataBase 时调用 database_manager.load_bar_data / load_tick_data；策略的 load_bar/load_tick 在回测内部同样走 database_manager 或 sqglobal 的 history_bar/history_tick。
- **最新一条**：get_newest_bar_data / get_newest_tick_data 用于增量更新或断点续传场景。

## 5. 小结

Data 模块提供：**与驱动无关的 Bar/Tick 持久化接口**、**SQL 与 MongoDB 两种实现**、**从 Tick 合成 K 线的 BarGenerator**、**K 线序列与技术指标 ArrayManager**、**RQData 历史 Bar 查询**。理解本模块即可理解回测与录播的数据来源与落库方式。
