# Engine 模块说明

## 1. 模块概述

`source/engine/` 实现**事件驱动**的引擎层：事件引擎负责事件队列与分发；策略引擎负责实盘/模拟下的策略管理、订单路由与回报处理；回测引擎负责历史数据回放、撮合与统计；录播引擎负责行情录制与 K 线合成。GUI 与 C++ 服务端通过 nanomsg 与策略/录播引擎通信。

## 2. 目录结构

```
source/engine/
├── __init__.py
├── iengine.py          # EventEngine、BaseEngine
├── strategy_engine.py  # 策略管理、下单、回报、与服务器通信
├── backtest_engine.py  # 回测与参数优化
└── recorder_engine.py # 行情录制与 Bar 合成
```

## 3. 各文件功能与逻辑

### 3.1 `iengine.py`

- **EventEngine**  
  - 维护一个队列和按 `EventType` 分组的 handler 列表；单独线程从队列取 Event，根据 `event_type` 分发给已注册的 handler；另有一定时器线程按 `interval` 秒向队列投递 `EventType.TIMER`。  
  - `put(event)`：投递事件；`register(type, handler)` / `unregister(type, handler)`：按类型注册/注销；`register_general` / `unregister_general`：注册/注销“接收所有类型”的 handler。  
  - 用于：GUI 端处理来自 ClientMq 的行情/订单/持仓等；策略引擎、录播引擎内部处理 Tick、订单回报、控制命令等。

- **BaseEngine**  
  - 抽象基类，持有 `event_engine`（可外部传入或新建），提供 `start()`（启动事件引擎）、`close()`。  
  - StrategyEngine、RecorderEngine 继承 BaseEngine，在 start 时同时启动事件引擎和各自的 recv 线程。

### 3.2 `strategy_engine.py`

- **StrategyEngine(BaseEngine)**  
  - 角色：实盘/模拟场景下的“策略+交易”中枢，通过 nanomsg 的 SUB 收服务器推送、PUSH 发请求；内部用 EventEngine 分发收到的消息。

- **初始化与配置**  
  - 读取 `etc/config_server.yaml`（含 serverpub_url、serverpull_url 等）；加载 `etc/ctpcontract.yaml` 得到 ContractData 字典；加载策略类（load_strategy_class）、策略配置与运行时数据（load_strategy_setting / load_strategy_data）；初始化 RQData（init_rqdata）；创建 OffsetConverter（trade.portfolio_manager），用于平仓时的开平今昨转换。

- **事件注册**  
  - 注册 TICK、ORDERSTATUS、FILL、POSITION、ACCOUNT、CONTRACT、STRATEGY_CONTROL、HEADER、TIMER 等类型的 handler，分别调用 process_tick_event、process_orderstatus_event、process_trade_event 等。

- **策略管理**  
  - 维护 strategies（name → 策略实例）、symbol_strategy_map（合约 → 策略列表）、orderid_strategy_map（订单 → 策略）；支持 init/start/stop/remove 等控制消息（MSG_TYPE_STRATEGY_*）。  
  - 定时器里在 8:55/20:55 附近自动 init 策略、8:57/20:57 自动 start，16:00/3:00 附近自动 stop，实现按交易时段启停。

- **订单与回报**  
  - 发单：策略调用 send_order → 经 RiskManager、OffsetConverter（需要时拆平今/平昨）→ 序列化为 Event 通过 PUSH 发给服务器；订单与策略的映射用 client_order_id 等维护。  
  - 回报：收到 RTN_ORDER/RTN_TRADE/RSP_POS 等后反序列化为 OrderData/TradeData/PositionData，更新本地 orders/trades/positions，并推给 OffsetConverter 和对应策略的 on_order/on_trade/on_pos。

- **数据与合约**  
  - get_contract、get_tick、get_order、get_position、get_account 等供策略查询；load_bar/load_tick 可走 database_manager 或 rqdata_client，结果通过回调推给策略。

### 3.3 `backtest_engine.py`

- **BacktestingEngine**  
  - 角色：加载历史 Bar 或 Tick，按时间推进回放，驱动策略的 on_bar/on_tick；内部维护持仓（PositionHolding）、限价单/止损单池，每根 Bar 或每个 Tick 做一次撮合与每日结算，最后统计收益、回撤、胜率等。

- **参数与模式**  
  - `set_parameters(...)`：设置合约、周期、起止时间、手续费率、滑点、合约乘数、最小变动、初始资金、回测模式（Bar/Tick）。  
  - `add_strategy(strategy_class, setting)`：实例化策略，并把策略的 write_log 重定向到引擎的 output（如打印或写入 logs 列表）。  
  - `load_data(datasource)`：若为 DataBase 则从 database_manager 加载 Bar 或 Tick；若为 Memory 则从 sqglobal.history_bar/history_tick 取对应合约列表并做时间切片，得到 history_data 与 startix/endix。

- **回放流程**  
  - `run_backtesting()`：先执行策略 on_init；若有 historybar_callback/historytick_callback（由策略 load_bar/load_tick 设置），则先回放一段历史用于策略初始化；然后 `strategy.on_start()`、`trading=True`，再按 history_data 顺序调用 new_bar 或 new_tick。  
  - `new_bar` / `new_tick`：更新当前 bar/tick 与 datetime；`cross_limit_order()` 与 `cross_stop_order()` 用当前价撮合限价单与止损单，产生 BacktestTradeData、更新 PositionHolding、调用 strategy.on_order/on_trade；`update_daily_close(price)` 按交易日（考虑夜盘归属）更新 daily_results 与持仓结算价。

- **结算与统计**  
  - `calculate_result()`：将每笔成交归入对应交易日（按 17:00 与 8:00 规则），生成 DailyResult 列表并计算每日净盈亏、手续费、滑点等，得到 daily_df。  
  - `calculate_statistics(df)`：基于 daily_df 算总收益、年化、最大回撤、夏普、胜率等，可打印或返回字典。  
  - `show_chart(df)`：用 matplotlib 画资金曲线、回撤、每日盈亏分布等。

- **策略用历史数据**  
  - 策略在 on_init 里可调用 `load_bar(days, interval, callback)` / `load_tick(days, callback)`；引擎内部从 database 或 Memory 取对应区间数据，在 run_backtesting 开始时先对 strategy 执行这些 callback，再跑主回放，从而支持“用过去 N 天数据初始化指标再回测”。

- **参数优化**  
  - **OptimizationSetting**：通过 add_parameter(name, start, end, step) 定义参数网格，generate_setting() 得到所有组合。  
  - `run_optimization(optimization_setting)`：多进程池对每组参数跑一次回测，按 target_name（如 sharpe_ratio、total_return）排序输出。  
  - `run_ga_optimization(...)`：用 DEAP 的遗传算法（eaMuPlusLambda）在参数空间搜索，适应度函数为单次回测的 target 指标；返回 ParetoFront 上的参数与目标值。

- **辅助函数**  
  - 模块级 `load_bar_data` / `load_tick_data`：封装 database_manager 的 load 接口，供 BacktestingEngine 和策略 load_bar/load_tick 使用。

### 3.4 `recorder_engine.py`

- **RecorderEngine(BaseEngine)**  
  - 角色：订阅行情，将 Tick 写入数据库，并用 BarGenerator 合成 Bar 后写入数据库；通过 nanomsg 与服务器通信，接收订阅与控制命令。

- **初始化**  
  - 读取 config_server.yaml；加载 ctpcontract.yaml 得到 contracts；从 data_recorder_setting.json 加载 tick_recordings、bar_recordings（要录的合约列表）；init_nng 连接 serverpub/serverpull；注册 TICK、CONTRACT、RECORDER_CONTROL、HEADER、TIMER。

- **订阅与录制**  
  - 定时器在 8:50/20:50 附近自动 init_subcribe（对 tick_recordings 与 bar_recordings 中的合约发订阅）；收到 Tick 后，在交易时段内：若 full_symbol 在 tick_recordings 则 record_tick(tick) → database_manager.save_tick_data；若在 bar_recordings 则取或创建该合约的 BarGenerator，update_tick(tick)，BarGenerator 的 on_window_bar 回调里 record_bar(bar) → save_bar_data。  
  - add_tick_recording / add_bar_recording：将合约加入配置并订阅、落库；remove_* 从配置移除；clear 清空配置并 save。

- **控制消息**  
  - RECORDER_STATUS：返回运行状态；RECORDER_ADD_TICK/BAR、REMOVE_*：增删录制合约；RECORDER_START：init_subcribe；RECORDER_STOP：clear；RECORDER_GET_DATA：向请求方返回当前 tick/bar 列表（put_event 序列化发送）。

- **运行**  
  - start()：启动事件引擎并启动 _run 线程，循环 recv 消息、反序列化为 Event、put 到 event_engine；stop() 停止线程与事件引擎。

## 4. 与其它模块的关系

- **common**：Event、EventType、MSG_TYPE、各类 Data、constant、utility 被大量使用。  
- **data**：database_manager 用于加载/保存 Bar 与 Tick；BarGenerator、load_bar_data/load_tick_data 用于录播与回测。  
- **strategy**：StrategyBase 由 StrategyEngine 与 BacktestingEngine 加载与驱动；策略内 get_position_holding、send_order 等由引擎注入。  
- **trade**：OffsetConverter、PositionHolding 被 StrategyEngine 与 BacktestingEngine 使用，用于平仓转换与持仓、冻结计算。

## 5. 小结

Engine 模块提供：**事件引擎**（队列+按类型分发+定时器）、**策略引擎**（实盘/模拟的策略生命周期、下单、回报、与服务器通信）、**回测引擎**（历史回放、限价/止损撮合、每日结算、统计与图表、网格/遗传优化）、**录播引擎**（Tick/Bar 录制与持久化）。理解本模块即可掌握策略在实盘与回测下的运行方式以及行情数据的写入与读取路径。
