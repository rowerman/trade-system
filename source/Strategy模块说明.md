# Strategy 模块说明

## 1. 模块概述

`source/strategy/` 提供**策略基类与下单封装**：StrategyBase 定义策略生命周期（on_init/on_start/on_stop）、行情与订单回报回调（on_tick/on_bar/on_order/on_trade 等），并封装 buy_open/sell_close 等便捷下单接口；支持 CTP 与 PAPER（模拟）两种 API，通过 CtpOrderField 与 PaperOrderField 组单。同时提供 CtaSignal、TargetPosTemplate、Strategies 等辅助类，供实盘引擎与回测引擎统一驱动。

## 2. 目录结构

```
source/strategy/
├── __init__.py
└── strategy_base.py   # StrategyBase、CtaSignal、TargetPosTemplate、Strategies
```

## 3. 核心类功能与逻辑

### 3.1 StrategyBase

- **角色**：所有 CTA/实盘/回测策略的基类，由 StrategyEngine 或 BacktestingEngine 创建并注入 engine 引用；策略通过 engine 的 get_position_holding、send_order、cancel_order、load_bar、load_tick 等与外界交互。

- **类属性**  
  - ID、NAME、author、account、api、autostart、parameters、variables：用于 UI 展示与参数持久化；parameters 列表中的名字会从 setting 字典写回实例属性。

- **构造**  
  - 参数：strategy_engine（或 BacktestingEngine）、strategy_name、full_symbol、setting。  
  - 解析 full_symbol 得到 symbol；保存 engine_id、strategy_name、full_symbol；初始化 active、inited、trading、pos 及多空持仓与价格（long_pos、short_pos、long_price 等）；copy variables 并插入 inited、trading、pos；update_setting(setting)；add_functions() 将 engine 的 get_position_holding、get_account、get_order、get_tick、get_trade、get_position、get_contract、get_all_active_orders 等绑定到策略实例。

- **生命周期（虚方法，子类可重写）**  
  - on_init(params_dict)：初始化时调用，可在此 load_bar/load_tick 预加载历史。  
  - on_start()：默认置 active=True。  
  - on_stop()：默认置 active=False。  
  - on_reset()：重置状态。

- **行情与回报回调**  
  - on_tick(tick)、on_bar(bar)：行情驱动；回测时由 BacktestingEngine 在 new_tick/new_bar 里调用。  
  - on_order(order)、on_order_status(order)：订单状态更新，默认 on_order 调用 on_order_status。  
  - on_cancel(event)、on_fill(trade)、on_trade(trade)：撤单、成交（on_trade 即 on_fill）。  
  - on_pos(position)、on_acc(acc)、on_contract(contract)、on_info(info)、on_req(req)、on_headermsg(event)、on_stop_order(stop_order)：持仓、账户、合约、日志、请求、止损单等。

- **持仓与订单查询**  
  - get_my_active_orderids()：返回本策略当前未完成订单 id 集合。  
  - get_my_position_holding()：从 engine 取当前 account+full_symbol 的 PositionHolding，并同步到策略的 long_pos、short_pos、long_price 等。

- **下单封装（核心逻辑）**  
  - buy_open(price, size, type)、buy_close、sell_open、sell_close：type 可为 'mkt'、'lmt'、'fak'、'fok'。  
  - 根据 self.api 分支：  
    - **CTP.TD**：构造 CtpOrderField（InstrumentID、OrderPriceType、Direction、CombOffsetFlag、CombHedgeFlag、LimitPrice、VolumeTotalOriginal、TimeCondition、VolumeCondition、MinVolume、ContingentCondition 等），使用 api.ctp_constant 中的 THOST_FTDC_* 常量；再构造 OrderRequest（OrderData 形态），orderfield=of，调用 strategy_engine.send_order(self, order)。  
    - **PAPER.TD**：构造 PaperOrderField（order_type、full_symbol、order_flag、limit_price、stop_price、order_size），同样封装为 OrderRequest 后 send_order。  
  - 开平与方向对应：buy_open 为多开，sell_close 为多平；sell_open 为空开，buy_close 为空平；PAPER 侧 sell 时 order_size 传负数表示空头。  
  - cancel_order(oid)、cancel_all()：仅在 trading 为 True 时调用 engine.cancel_order/cancel_all。

- **兼容接口（vnpy 风格）**  
  - buy(price, volume)、sell、short、cover：内部转调 buy_open、sell_close、sell_open、buy_close。

- **其它**  
  - get_parameters()、get_variables()、get_data()：供 UI 或持久化使用。  
  - write_log(msg)、put_event()、send_email(msg)、sync_data()、load_bar(days, interval, callback, datasource)、load_tick(days, datasource)：委托给 engine 或占位。  
  - get_engine_type()：返回实盘或回测。

### 3.2 CtaSignal

- 抽象基类，维护 signal_pos；提供 on_tick、on_bar 虚方法及 set_signal_pos、get_signal_pos。用于仅产生信号的子模块（如技术指标信号），由策略组合使用。

### 3.3 TargetPosTemplate(CtaTemplate)

- 继承 StrategyBase，增加 target_pos、last_tick、last_bar、vt_orderids；variables 中追加 "target_pos"。  
- on_tick/on_bar 中更新 last_tick/last_bar，若 trading 则调用 trade()。  
- set_target_pos(target_pos)：设置目标仓位并调用 trade()。  
- trade()：先 cancel_all()，再根据 target_pos 与当前 pos 的差值、last_tick 或 last_bar 的买卖价，计算 long_price/short_price，内部有注释掉的 buy/short/cover/sell 调用逻辑，实际项目中可打开或按需实现，用于“目标持仓”类策略。

### 3.4 Strategies

- 将多个策略实例组合成一个“策略集合”；on_tick(event) 中遍历 _strategies_collection 依次调用各策略的 on_tick。用于多策略叠加或分层执行。

## 4. 与引擎的协作

- **StrategyEngine**：创建策略实例时传入 self；策略 send_order 时 engine 做风控、OffsetConverter 转换后通过 nanomsg 发单；回报通过 event 回调策略 on_order/on_trade；load_bar/load_tick 由 engine 从 database 或 rqdata 取数并回调。  
- **BacktestingEngine**：创建策略时传入 self 作为 strategy_engine；策略的下单被引擎转为内部限价/止损单池，在 new_bar/new_tick 中撮合；load_bar/load_tick 由回测引擎从 history_data 或 database 提供一段历史并在 run_backtesting 开始时回放给策略。

## 5. 小结

Strategy 模块提供：**统一的策略基类 StrategyBase**（生命周期、行情与订单回报、持仓与账户查询）、**针对 CTP 与模拟盘的报单封装**（CtpOrderField/PaperOrderField）、**目标持仓模板与多策略组合**。理解本模块即可掌握如何编写策略、如何通过 engine 下单与获取数据，以及实盘与回测如何共用同一套策略接口。
