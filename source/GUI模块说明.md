# GUI 模块说明

## 1. 模块概述

`source/gui/` 使用 **PyQt5** 实现 StarQuant 的桌面图形界面：主窗口、菜单、状态栏、行情/订单/成交/持仓/账户/日志等监控、策略管理、手工下单、数据录播管理、回测配置与结果展示、数据导入与下载等。GUI 通过 **ClientMq**（common.client_mq）与后端服务（C++ 等）以 nanomsg 通信，通过 **EventEngine** 接收服务器推送并更新界面。

## 2. 目录结构

```
source/gui/
├── __init__.py
├── ui_main_window.py    # 主窗口、菜单、事件注册、ClientMq
├── ui_basic.py          # 表格单元格、TabBar、颜色常量等基础控件
├── ui_common_widget.py  # 录播/合约/数据加载/关于/Web/对话框等通用组件
├── ui_monitors.py       # 行情、订单、成交、持仓、账户、日志监控表
├── ui_strategy_window.py # CTA 策略管理（加载类、添加/启停策略、参数）
├── ui_manual_window.py  # 手工下单窗口
├── ui_dataview.py       # 行情数据查看（如列表/图表）
├── ui_bt_setting.py     # 回测配置、运行、优化、结果展示（BacktesterManager）
├── ui_bt_dataview.py    # 回测用行情/曲线图（BTQuotesChart 等）
├── ui_bt_resultsoverview.py # 回测结果概览与图表（BacktesterChart）
├── ui_bt_txnview.py     # 回测成交与逐日表格（TradesTable、DailyTable）
├── image/               # 图标资源（logo.ico、star.png）
└── language/            # 多语言文案（cn/en 的 live_text.yaml~）
```

## 3. 各文件功能与逻辑

### 3.1 `ui_main_window.py`

- **MainWindow(QMainWindow)**  
  - 构造时接收 config_server、config_client、lang_dict；创建 EventEngine、outgoing_queue、ClientMq、RiskManager（PassThroughRiskManager）、ContractManager、RecorderManager、DataDownloaderWidget、BacktesterManager（回测）、CtaManager（策略）、ManualWindow、MarketDataView 等。  
  - **init_menu**：菜单项（文件、功能、数据、回测、帮助等），用于打开各子窗口或对话框。  
  - **init_status_bar**：状态栏显示时间、连接状态等。  
  - **init_central_area**：中央区域可放置 Tab 或默认欢迎页。  
  - **事件注册**：将 TICK、ORDERSTATUS、FILL、POSITION、ACCOUNT、CONTRACT、INFO、STRATEGY_CONTROL、ENGINE_CONTROL、RECORDER_CONTROL、BACKTEST_LOG 等 EventType 注册到 _events_engine，对应 _tick_event_handler、_order_status_event_handler 等，在 handler 里更新对应监控窗口的数据（若已打开）。  
  - **ClientMq**：start 后启动接收线程，把服务器消息反序列化为 Event 并 put 到 _events_engine；用户操作（订阅、下单、策略控制、录播控制等）由各子窗口组包为请求放入 _outgoing_queue，由 ClientMq 的 _run 发送。  
  - 流程：服务器 → SUB 收 → EventEngine.put → 各 monitor 更新；用户点击 → 组 Event/请求 → outgoing_queue → ClientMq PUSH 发。

### 3.2 `ui_basic.py`

- **VerticalTabBar**：自定义 TabBar 绘制（竖排标签等）。  
- **QFloatTableWidgetItem**：支持按数值排序的表格项。  
- **BaseCell**：通用表格单元格，set_content(content, data)，便于统一显示与排序。  
- **颜色常量**：COLOR_LONG、COLOR_SHORT、COLOR_BID、COLOR_ASK 等，用于买卖方向、盘口着色。  
- 其它基础控件或工具类供 monitors、strategy、bt 等复用。

### 3.3 `ui_common_widget.py`

- **RecorderManager**：配置要录制的 Tick/Bar 合约列表，发出增删请求（signal_recorder_out），由主窗口转发到 outgoing_queue（RECORDER_ADD_TICK/BAR、REMOVE_* 等）。  
- **ContractManager**：加载合约列表（如从 yaml），供录播、手工下单等选择合约。  
- **StatusThread**：可能用于定时拉取或刷新状态。  
- **CsvLoaderWidget**：从 CSV 导入 Tick（列名映射、时间格式、symbol/exchange），可选写入 DataBase 或 Memory（sqglobal.history_tick），供回测使用。  
- **DataDownloaderWidget**：可能封装从 RQData 等下载历史数据的界面。  
- **AboutWidget、WebWindow、GlobalDialog、TextEditDialog**：关于页、内嵌浏览器、通用对话框等。

### 3.4 `ui_monitors.py`

- **MarketMonitor**：行情监控表，订阅 TICK 事件，按合约显示最新价、买卖盘、量等。  
- **OrderMonitor**：订单监控，订阅 ORDERSTATUS，显示委托列表与状态。  
- **TradeMonitor**：成交监控，订阅 FILL，显示成交明细。  
- **PositionMonitor**：持仓监控，订阅 POSITION，显示多空、数量、盈亏等。  
- **AccountMonitor**：账户监控，订阅 ACCOUNT，显示权益、保证金、可用等。  
- **LogMonitor**：日志监控，订阅 INFO 或 BACKTEST_LOG，显示运行日志。  
- 各 Monitor 通常用 QTableWidget 或类似结构，在对应 event handler 里插入/更新行，并可能支持排序、过滤。

### 3.5 `ui_strategy_window.py`

- **CtaManager**：策略管理界面。  
  - 从 `mystrategy` 目录扫描 .py，通过 importlib 加载继承 StrategyBase 的类，填充 classes（类名→类）。  
  - 读取 cta_strategy_setting.json、cta_strategy_data.json，维护 strategy_setting、strategy_data。  
  - UI：策略类下拉、合约选择、参数编辑、添加/初始化/启动/停止/移除策略等按钮。  
  - 用户操作序列化为 Event（MSG_TYPE_STRATEGY_ADD、STRATEGY_INIT、STRATEGY_START 等），通过 signal_strategy_out 或类似机制交给主窗口，主窗口放入 outgoing_queue 发到服务器；服务器由 StrategyEngine 处理并可能通过 STRATEGY_CONTROL/INFO 回传，主窗口再通过 event 更新界面或日志。  
  - 支持多引擎/多策略名与 engine_id 的映射（strategy_engine_map）。

### 3.6 `ui_manual_window.py`

- **ManualWindow**：手工下单。选择合约、方向、开平、价格、数量、订单类型等，组单后通过主窗口的请求通道发送下单请求（MSG_TYPE_ORDER 等），并可能显示委托与成交结果（依赖 ORDERSTATUS、FILL 事件）。

### 3.7 `ui_dataview.py`

- **MarketDataView**：行情数据查看，可能包含合约列表、历史 Bar/Tick 列表或简单图表，数据来源可为 database_manager 或 sqglobal，用于回测前检查或浏览数据。

### 3.8 `ui_bt_setting.py`

- **BacktesterManager / Backtester**：  
  - 策略类加载（与 CtaManager 类似，从 mystrategy 加载）。  
  - 回测参数：合约、周期、起止时间、手续费、滑点、合约乘数、最小变动、资金、Bar/Tick 模式、数据源（DataBase/Memory）。  
  - **init_engine**：创建 BacktestingEngine，将 output 重定向到 write_log（通过 EventEngine 发 BACKTEST_LOG，由 LogMonitor 或回测页内文本显示）。  
  - 运行回测：在子线程中调用 backtesting_engine.load_data、run_backtesting、calculate_result、calculate_statistics，结果写回 result_df、result_statistics、result_trades、result_dailys。  
  - 参数优化：调用 run_optimization 或 run_ga_optimization，结果在界面展示。  
  - 与 ui_bt_resultsoverview、ui_bt_dataview、ui_bt_txnview 配合：结果曲线（BacktesterChart）、回测行情/曲线（BTQuotesChart）、成交与逐日表（TradesTable、DailyTable）。

### 3.9 `ui_bt_resultsoverview.py`

- **BacktesterChart**：基于回测结果 DataFrame 绘制资金曲线、回撤、每日盈亏等图表（可能使用 pyqtgraph 或 matplotlib），供回测页展示。

### 3.10 `ui_bt_dataview.py`

- **BTQuotesChart**：回测过程中的行情或净值曲线展示，可能与 BacktestingEngine 的 show_chart 或 daily_df 对接。

### 3.11 `ui_bt_txnview.py`

- **TradesTable**：回测成交明细表。  
- **DailyTable**：按交易日的汇总表（每日盈亏、手续费等）。  
- 数据来自 Backtester 的 result_trades、result_dailys 或 backtesting_engine 的 trades、daily_df。

## 4. 事件流小结

- **入站**：ClientMq 收 → EventEngine.put(Event) → 按 event_type 调用已注册 handler → 更新对应 Monitor 或策略/回测界面。  
- **出站**：用户在各窗口操作 → 构造 Event 或请求体 → 主窗口放入 _outgoing_queue 或通过 signal 交给主窗口再放入 queue → ClientMq._run 从 queue 取并 PUSH 发送。

## 5. 依赖关系

- **common**：Event、EventType、MSG_TYPE、datastruct、config、client_mq、constant、utility。  
- **engine**：EventEngine、BacktestingEngine、OptimizationSetting。  
- **strategy**：StrategyBase。  
- **data**：database_manager、rqdata_client。  
- **trade**：PassThroughRiskManager。  
- **api**：仅在 common/strategy 中间接使用。

## 6. 小结

GUI 模块提供：**主窗口与事件分发**、**与后端通信的 ClientMq 集成**、**行情/订单/成交/持仓/账户/日志监控**、**策略管理（CTA）与手工下单**、**录播与合约管理**、**回测配置、运行、优化与结果展示**、**数据导入与下载**。理解本模块即可掌握从界面操作到服务器请求、以及从服务器推送到界面更新的完整路径。
