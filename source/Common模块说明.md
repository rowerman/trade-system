# Common 模块说明

## 1. 模块概述

`source/common/` 是项目的**公共基础层**，提供配置加载、全局常量、事件与业务数据结构、消息队列客户端、全局状态以及通用工具函数。其他模块（data、engine、strategy、trade、gui）均依赖本模块。

## 2. 目录结构

```
source/common/
├── __init__.py
├── config.py      # 配置与 SETTINGS
├── constant.py    # 枚举与常量（消息类型、方向、订单状态等）
├── datastruct.py  # Event、Tick/Bar/Order/Trade/Position 等数据结构
├── client_mq.py   # 与服务器通信的 MQ 客户端（nanomsg）
├── sqglobal.py    # 全局上下文与历史数据容器
└── utility.py     # 路径、JSON、符号转换等工具
```

## 3. 各文件功能与逻辑

### 3.1 `config.py`

- **SETTINGS**：内置默认配置字典，包括字体、日志、邮件、rqdata 账号、数据库类型与连接信息等。
- **SETTING_FILENAME**：`sq_setting.json`，从 `utility.get_file_path()` 指向的路径（如 `.StarQuant` 目录）加载，并 **update** 到 SETTINGS。
- **get_settings(prefix)**：按前缀筛选配置项并去掉前缀，例如 `get_settings("database.")` 返回数据库相关键值，供 data 模块初始化数据库使用。

### 3.2 `constant.py`

定义全项目统一的枚举与常量，便于与 CTP 和内部逻辑对应：

- **ESTATE**：连接/认证/登录等状态机。
- **SYMBOL_TYPE**：FULL / CTP 等合约标识方式。
- **MSG_TYPE**：消息类型编号（10\* 数据、11\* 引擎、12\* 策略、14\* 录播、20\* 订阅/下单/回报、31\* 信息、34\* 错误等）。
- **EventType**：事件类型（TICK、BAR、ORDERSTATUS、FILL、POSITION、STRATEGY_CONTROL 等），供事件引擎分发。
- **OrderFlag / OrderType / OrderStatus**：订单开平、价格类型、订单状态。
- **TickType**：Tick/Bar 数据类型。
- **Direction / Offset**：多空、开平（含平今/平昨），与 CTP 的 `THOST_FTDC_*` 通过 `DIRECTION_VT2CTP`、`ORDERFALG_2VT` 等映射。
- **OptionType / Product / Exchange / Currency / Interval**：期权类型、品种、交易所、货币、K 线周期。
- **Status**：面向 UI 的订单状态（等待提交、部分成交、已撤销等）。
- **EngineType / BacktestingMode**：实盘/回测、Bar 回测/Tick 回测。
- **StopOrderStatus**：止损单状态。

业务逻辑依赖这些常量做分支判断和序列化，例如 datastruct 中按 `MSG_TYPE` 反序列化不同 Event 类型。

### 3.3 `datastruct.py`

提供事件与业务数据的**基类与具体类型**，并实现序列化/反序列化，用于进程间或 GUI 与引擎间的消息传递。

- **Event**  
  - 字段：`event_type`、`data`、`destination`、`source`、`msg_type`。  
  - `serialize()`：拼接为 `des|src|msg_type_value|data_serialized` 的字符串。  
  - `deserialize(msg)`：按 `|` 分割，根据 `msg_type` 解析为 Tick、Order、Trade、Position、Bar、Account、Contract、Log 等，并设置 `event_type` 与 `data`。

- **BaseData**：带 `gateway_name` 的数据基类。

- **TickData**：行情 Tick（合约、交易所、时间、最新价、量、五档买卖、涨跌停、昨收、持仓量等）；`__post_init__` 中生成 `vt_symbol`、`full_symbol`、`timestamp`；`deserialize` 支持 1 档与 5 档两种格式。

- **BarData**：K 线（开高低收、量、周期、持仓量等）；`__post_init__` 生成 `vt_symbol`、`full_symbol`、`bar_start_time`。

- **OrderData**：订单（合约、方向、开平、价格、数量、已成交、状态、CTP 相关 id、orderfield 等）；`is_active()`、`create_cancel_request()`；`deserialize` 与 CTP 回报格式对应。

- **CtpOrderField / PaperOrderField**：CTP 与模拟盘下单字段，带 `serialize()`，供 strategy 组单、engine 转发。

- **TradeData**：成交（合约、订单 id、成交 id、方向、开平、价格、数量、手续费等）；回测用字段如 `commission`、`slippage`、`turnover`、多空持仓与盈亏。

- **PositionData**：持仓（合约、多空、数量、冻结、价格、盈亏、昨仓等）。

- **AccountData**：账户（余额、冻结、可用、权益、保证金、平仓盈亏、持仓盈亏等）。

- **LogData**：日志（消息、级别、时间戳）。

- **ContractData**：合约（symbol、exchange、名称、品种、乘数、最小变动、净持仓、期权相关、保证金比例等）；`deserialize` 与 CTP 合约信息对应。

- **StopOrder**：止损单（合约、方向、开平、价格、数量、策略名、状态等）。

- **QryContractRequest / SubscribeRequest / CancelRequest / HistoryRequest**：查询合约、订阅、撤单、历史数据的请求体。

逻辑要点：**Event + 各类 Data** 构成“消息体”，通过 `client_mq` 或 engine 的 nanomsg 在客户端与服务器之间传递；反序列化时根据 `msg_type` 分支，保证 GUI 与策略引擎拿到的是强类型数据。

### 3.4 `client_mq.py`

- **ClientMq**：封装与后端的**双路通信**。  
  - 构造：传入 config（含 server 的 pub/pull 等 URL）、ui 的 event_engine、outgoing_queue。  
  - **接收**：SUB socket 收服务器推送，将字符串反序列化为 `Event` 并 `put` 到 ui 的 event_engine。  
  - **发送**：从 outgoing_queue 取消息，通过 PUSH socket 发给服务器。  
  - `start()`：创建 SUB/PUSH（及 MONITOR 用 SUB），设置超时与订阅，启动 `_run` 线程。  
  - `stop()`：置位停止并 join 线程。

GUI 主窗口在初始化时创建 ClientMq，将用户操作（如订阅、下单、策略控制）放入 outgoing_queue，把服务器推送的行情、订单、持仓等通过 event_engine 交给各监控窗口。

### 3.5 `sqglobal.py`

- **dotdict**：可用“点号”访问的字典，用于全局上下文。  
- **context**：全局 dotdict，策略等可读写。  
- **history_bar / history_tick**：`defaultdict(list)`，按合约存储历史 Bar/Tick 列表，供回测或数据查看从“内存”加载（如 BacktestingEngine 的 datasource="Memory"）。

### 3.6 `utility.py`

- **extract_full_symbol / generate_full_symbol**：full_symbol 与 (symbol, exchange) 互转，兼容期货合约与组合格式。  
- **extract_vt_symbol / generate_vt_symbol**：vt_symbol（如 symbol.exchange）的解析与生成。  
- **_get_trader_dir / TRADER_DIR, TEMP_DIR**：获取运行目录或用户目录下的 `.StarQuant` 路径。  
- **get_file_path / get_folder_path / get_icon_path**：在 TEMP_DIR 下取文件/目录/图标路径。  
- **load_json / save_json**：在 TEMP_DIR 下读写 JSON（供 config、策略设置等使用）。  
- **round_to_pricetick / round_to**：价格按最小变动价位取整。  
- **virtual**：装饰器，标记可被子类重写的“虚函数”。

## 4. 数据流与依赖关系

- **config** 被 data、engine、gui 等读取，决定数据库、日志、rqdata 等行为。  
- **constant** 被 datastruct、engine、strategy、trade 广泛使用，保证类型与消息号统一。  
- **datastruct** 的 Event 与各 Data 是“消息体”和“业务对象”的唯一定义，client_mq 与各 engine 只做收发与反序列化。  
- **client_mq** 仅被 GUI 使用，连接后端与前端事件引擎。  
- **sqglobal** 主要被回测或数据加载逻辑使用。  
- **utility** 被 config、datastruct、engine、strategy 等调用，无反向依赖。

## 5. 小结

Common 模块提供：**配置与常量**、**事件与业务数据结构及序列化**、**GUI 与后端的 MQ 客户端**、**全局上下文与历史数据容器**、**路径与 JSON 等工具**。理解本模块后，即可理解整个项目在“类型、消息、配置”上的约定，以及 GUI 与服务器之间的数据形态。
